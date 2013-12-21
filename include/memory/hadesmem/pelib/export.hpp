// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstddef>
#include <iosfwd>
#include <limits>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>

#include <windows.h>
#include <winnt.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/str_conv.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/pelib/export_dir.hpp>
#include <hadesmem/pelib/nt_headers.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

// TODO: Constructor to create Export by name (optimize using binary
// search).

// TODO: Support setting and writing back Export. (For EAT hooking.)

// TODO: Test that export code works against ordinal-only modules. From
// Corkami:
// ordinals-only exports can make the structure even smaller (no
// NumberOfFunctions/NumberOfNames/AddressOfNames/AddressOfNameOrdinals).
// Fake entries can be also present in exports as long as Base + Ordinal
// matches the wanted export.

// TODO: Fix the code so this hack can be removed.
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextended-offsetof"
#endif

namespace hadesmem
{

class Export
{
public:
  explicit Export(Process const& process,
                  PeFile const& pe_file,
                  WORD procedure_number)
    : process_(&process),
      pe_file_(&pe_file),
      rva_(0),
      va_(nullptr),
      name_(),
      forwarder_(),
      forwarder_split_(),
      procedure_number_(procedure_number),
      ordinal_number_(0),
      by_name_(false),
      forwarded_(false)
  {
    ExportDir const export_dir(process, pe_file);

    // TODO: Check for underflow here.
    ordinal_number_ =
      static_cast<WORD>(procedure_number_ - export_dir.GetOrdinalBase());

    if (ordinal_number_ >= export_dir.GetNumberOfFunctions())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                      << ErrorString("Ordinal out of range."));
    }

    WORD* const ptr_ordinals = static_cast<WORD*>(
      RvaToVa(process, pe_file, export_dir.GetAddressOfNameOrdinals()));
    if (!ptr_ordinals)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error() << ErrorString("AddressOfNameOrdinals invalid."));
    }

    DWORD* const ptr_names = static_cast<DWORD*>(
      RvaToVa(process, pe_file, export_dir.GetAddressOfNames()));
    if (!ptr_names)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error() << ErrorString("AddressOfNames invalid."));
    }

    if (DWORD const num_names = export_dir.GetNumberOfNames())
    {
      std::vector<WORD> const name_ordinals =
        ReadVector<WORD>(process, ptr_ordinals, num_names);
      auto const name_ord_iter = std::find(
        std::begin(name_ordinals), std::end(name_ordinals), ordinal_number_);
      if (name_ord_iter != std::end(name_ordinals))
      {
        by_name_ = true;
        DWORD const name_rva = Read<DWORD>(
          process,
          ptr_names + std::distance(std::begin(name_ordinals), name_ord_iter));
        name_ = ReadString<char>(process, RvaToVa(process, pe_file, name_rva));
      }
    }

    DWORD* const ptr_functions = static_cast<DWORD*>(
      RvaToVa(process, pe_file, export_dir.GetAddressOfFunctions()));
    if (!ptr_functions)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error() << ErrorString("AddressOfFunctions invalid."));
    }
    DWORD const func_rva =
      Read<DWORD>(process, ptr_functions + ordinal_number_);

    NtHeaders const nt_headers(process, pe_file);

    DWORD const export_dir_start =
      nt_headers.GetDataDirectoryVirtualAddress(PeDataDir::Export);
    DWORD const export_dir_end =
      export_dir_start + nt_headers.GetDataDirectorySize(PeDataDir::Export);

    // Check function RVA. If it lies inside the export dir region
    // then it's a forwarded export. Otherwise it's a regular RVA.
    if (func_rva > export_dir_start && func_rva < export_dir_end)
    {
      forwarded_ = true;
      forwarder_ =
        ReadString<char>(process, RvaToVa(process, pe_file, func_rva));

      std::string::size_type const split_pos = forwarder_.rfind('.');
      if (split_pos != std::string::npos)
      {
        forwarder_split_ = std::make_pair(forwarder_.substr(0, split_pos),
                                          forwarder_.substr(split_pos + 1));
      }
      else
      {
        HADESMEM_DETAIL_THROW_EXCEPTION(
          Error() << ErrorString("Invalid forwarder string format."));
      }
    }
    else
    {
      rva_ = func_rva;
      va_ = RvaToVa(process, pe_file, func_rva);
    }
  }

#if defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

  Export(Export const&) = default;

  Export& operator=(Export const&) = default;

  Export(Export&& other)
    : process_(other.process_),
      pe_file_(other.pe_file_),
      rva_(other.rva_),
      va_(other.va_),
      name_(std::move(other.name_)),
      forwarder_(std::move(other.forwarder_)),
      forwarder_split_(std::move(other.forwarder_split_)),
      procedure_number_(other.procedure_number_),
      ordinal_number_(other.ordinal_number_),
      by_name_(other.by_name_),
      forwarded_(other.forwarded_)
  {
  }

  Export& operator=(Export&& other)
  {
    process_ = other.process_;
    pe_file_ = other.pe_file_;
    rva_ = other.rva_;
    va_ = other.va_;
    name_ = std::move(other.name_);
    forwarder_ = std::move(other.forwarder_);
    forwarder_split_ = std::move(other.forwarder_split_);
    procedure_number_ = other.procedure_number_;
    ordinal_number_ = other.ordinal_number_;
    by_name_ = other.by_name_;
    forwarded_ = other.forwarded_;

    return *this;
  }

#endif // #if defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

  DWORD GetRva() const HADESMEM_DETAIL_NOEXCEPT
  {
    return rva_;
  }

  PVOID GetVa() const HADESMEM_DETAIL_NOEXCEPT
  {
    return va_;
  }

  std::string GetName() const
  {
    return name_;
  }

  WORD GetProcedureNumber() const HADESMEM_DETAIL_NOEXCEPT
  {
    return procedure_number_;
  }

  WORD GetOrdinalNumber() const HADESMEM_DETAIL_NOEXCEPT
  {
    return ordinal_number_;
  }

  bool ByName() const HADESMEM_DETAIL_NOEXCEPT
  {
    return by_name_;
  }

  bool ByOrdinal() const HADESMEM_DETAIL_NOEXCEPT
  {
    return !by_name_;
  }

  bool IsForwarded() const HADESMEM_DETAIL_NOEXCEPT
  {
    return forwarded_;
  }

  std::string GetForwarder() const
  {
    return forwarder_;
  }

  std::string GetForwarderModule() const
  {
    return forwarder_split_.first;
  }

  std::string GetForwarderFunction() const
  {
    return forwarder_split_.second;
  }

  bool IsForwardedByOrdinal() const
  {
    return (GetForwarderFunction()[0] == '#');
  }

  WORD GetForwarderOrdinal() const
  {
    if (!IsForwardedByOrdinal())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error() << ErrorString("Function is not exported by ordinal."));
    }

    try
    {
      std::string const forwarder_function(GetForwarderFunction());
      return detail::StrToNum<WORD>(forwarder_function.substr(1));
    }
    catch (std::exception const& /*e*/)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error() << ErrorString("Invalid forwarder ordinal detected."));
    }
  }

private:
  Process const* process_;
  PeFile const* pe_file_;
  DWORD rva_;
  PVOID va_;
  std::string name_;
  std::string forwarder_;
  std::pair<std::string, std::string> forwarder_split_;
  WORD procedure_number_;
  WORD ordinal_number_;
  bool by_name_;
  bool forwarded_;
};

inline bool operator==(Export const& lhs, Export const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetProcedureNumber() == rhs.GetProcedureNumber();
}

inline bool operator!=(Export const& lhs, Export const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(Export const& lhs, Export const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetProcedureNumber() < rhs.GetProcedureNumber();
}

inline bool operator<=(Export const& lhs, Export const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetProcedureNumber() <= rhs.GetProcedureNumber();
}

inline bool operator>(Export const& lhs, Export const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetProcedureNumber() > rhs.GetProcedureNumber();
}

inline bool operator>=(Export const& lhs, Export const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetProcedureNumber() >= rhs.GetProcedureNumber();
}

inline std::ostream& operator<<(std::ostream& lhs, Export const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << rhs.GetProcedureNumber();
  lhs.imbue(old);
  return lhs;
}

inline std::wostream& operator<<(std::wostream& lhs, Export const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << rhs.GetProcedureNumber();
  lhs.imbue(old);
  return lhs;
}
}

// TODO: Fix the code so this hack can be removed.
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic pop
#endif

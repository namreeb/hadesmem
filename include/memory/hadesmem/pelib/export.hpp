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
#include <hadesmem/detail/str_to_num.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/pelib/export_dir.hpp>
#include <hadesmem/pelib/nt_headers.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

// TODO: Fix the code so this hack can be removed.
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextended-offsetof"
#endif

namespace hadesmem
{

// TODO: Constructor to create Export by name (optimize using binary search).

// TODO: Support setting and writing back Export. (For EAT hooking.)

// TODO: Improve export forwarding code to detect and handle 
// forward-by-ordinal explicitly rather than forcing the user to detect it 
// and do string manipulation and conversion.

class Export
{
public:
  explicit Export(Process const& process, PeFile const& pe_file, 
    WORD ordinal)
    : process_(&process), 
    pe_file_(&pe_file), 
    rva_(0), 
    va_(nullptr), 
    name_(), 
    forwarder_(), 
    forwarder_split_(), 
    ordinal_(ordinal), 
    by_name_(false), 
    forwarded_(false)
  {
    ExportDir const export_dir(process, pe_file);

    WORD offset = static_cast<WORD>(ordinal_ - export_dir.GetOrdinalBase());
    
    if (offset >= export_dir.GetNumberOfFunctions())
    {
        HADESMEM_DETAIL_THROW_EXCEPTION(Error() << 
          ErrorString("Ordinal out of range."));
    }

    WORD* ptr_ordinals = static_cast<WORD*>(RvaToVa(process, pe_file, 
      export_dir.GetAddressOfNameOrdinals()));

    DWORD* ptr_names = static_cast<DWORD*>(RvaToVa(process, pe_file, 
      export_dir.GetAddressOfNames()));

    if (DWORD const num_names = export_dir.GetNumberOfNames())
    {
      std::vector<WORD> name_ordinals = ReadVector<WORD>(process, 
        ptr_ordinals, num_names);
      auto name_ord_iter = std::find(std::begin(name_ordinals), 
        std::end(name_ordinals), offset);
      if (name_ord_iter != std::end(name_ordinals))
      {
        by_name_ = true;
        DWORD const name_rva = Read<DWORD>(process, ptr_names + std::distance(
          std::begin(name_ordinals), name_ord_iter));
        name_ = ReadString<char>(process, RvaToVa(process, pe_file, name_rva));
      }
    }

    DWORD* ptr_functions = static_cast<DWORD*>(RvaToVa(process, pe_file, 
      export_dir.GetAddressOfFunctions()));
    DWORD const func_rva = Read<DWORD>(process, ptr_functions + offset);

    NtHeaders const nt_headers(process, pe_file);

    DWORD const export_dir_start = nt_headers.GetDataDirectoryVirtualAddress(
      PeDataDir::Export);
    DWORD const export_dir_end = export_dir_start + 
      nt_headers.GetDataDirectorySize(PeDataDir::Export);

    // Check function RVA. If it lies inside the export dir region 
    // then it's a forwarded export. Otherwise it's a regular RVA.
    if (func_rva > export_dir_start && func_rva < export_dir_end)
    {
      forwarded_ = true;
      forwarder_ = ReadString<char>(process, RvaToVa(process, pe_file, 
        func_rva));

      std::string::size_type split_pos = forwarder_.rfind('.');
      if (split_pos != std::string::npos)
      {
        forwarder_split_ = std::make_pair(forwarder_.substr(0, split_pos), 
          forwarder_.substr(split_pos + 1));
      }
      else
      {
        HADESMEM_DETAIL_THROW_EXCEPTION(Error() << 
          ErrorString("Invalid forwarder string format."));
      }
    }
    else
    {
      rva_ = func_rva;
      va_ = RvaToVa(process, pe_file, func_rva);
    }
  }
  
  Export(Export const& other)
    : process_(other.process_), 
    pe_file_(other.pe_file_), 
    rva_(other.rva_), 
    va_(other.va_), 
    name_(other.name_), 
    forwarder_(other.forwarder_), 
    forwarder_split_(other.forwarder_split_), 
    ordinal_(other.ordinal_), 
    by_name_(other.by_name_), 
    forwarded_(other.forwarded_)
  { }
  
  Export& operator=(Export const& other)
  {
    Export tmp(other);
    *this = std::move(tmp);

    return *this;
  }

  Export(Export&& other) HADESMEM_DETAIL_NOEXCEPT
    : process_(other.process_), 
    pe_file_(other.pe_file_), 
    rva_(other.rva_), 
    va_(other.va_), 
    name_(std::move(other.name_)), 
    forwarder_(std::move(other.forwarder_)), 
    forwarder_split_(std::move(other.forwarder_split_)), 
    ordinal_(other.ordinal_), 
    by_name_(other.by_name_), 
    forwarded_(other.forwarded_)
  { }
  
  Export& operator=(Export&& other) HADESMEM_DETAIL_NOEXCEPT
  {
    process_ = other.process_;
    pe_file_ = other.pe_file_;
    rva_ = other.rva_;
    va_ = other.va_;
    name_ = std::move(other.name_);
    forwarder_ = std::move(other.forwarder_);
    forwarder_split_ = std::move(other.forwarder_split_);
    ordinal_ = other.ordinal_;
    by_name_ = other.by_name_;
    forwarded_ = other.forwarded_;

    return *this;
  }

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

  WORD GetOrdinal() const HADESMEM_DETAIL_NOEXCEPT
  {
    return ordinal_;
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
      HADESMEM_DETAIL_THROW_EXCEPTION(Error() << 
        ErrorString("Function is not exported by ordinal."));
    }

    try
    {
      std::string const forwarder_function(GetForwarderFunction());
      return detail::StrToNum<WORD>(forwarder_function.substr(1));
    }
    catch (std::exception const& /*e*/)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error() << 
        ErrorString("Invalid forwarder ordinal detected."));
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
  WORD ordinal_;
  bool by_name_;
  bool forwarded_;
};

inline bool operator==(Export const& lhs, Export const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetOrdinal() == rhs.GetOrdinal();
}

inline bool operator!=(Export const& lhs, Export const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(Export const& lhs, Export const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetOrdinal() < rhs.GetOrdinal();
}

inline bool operator<=(Export const& lhs, Export const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetOrdinal() <= rhs.GetOrdinal();
}

inline bool operator>(Export const& lhs, Export const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetOrdinal() > rhs.GetOrdinal();
}

inline bool operator>=(Export const& lhs, Export const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetOrdinal() >= rhs.GetOrdinal();
}

inline std::ostream& operator<<(std::ostream& lhs, Export const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << rhs.GetOrdinal();
  lhs.imbue(old);
  return lhs;
}

inline std::wostream& operator<<(std::wostream& lhs, Export const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << rhs.GetOrdinal();
  lhs.imbue(old);
  return lhs;
}

}

// TODO: Fix the code so this hack can be removed.
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic pop
#endif

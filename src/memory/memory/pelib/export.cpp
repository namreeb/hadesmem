// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/pelib/export.hpp"

#include <cstddef>
#include <ostream>
#include <utility>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/assert.hpp>
#include <boost/lexical_cast.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include <windows.h>
#include <winnt.h>

#include "hadesmem/read.hpp"
#include "hadesmem/error.hpp"
#include "hadesmem/write.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/pelib/pe_file.hpp"
#include "hadesmem/pelib/export_dir.hpp"
#include "hadesmem/pelib/nt_headers.hpp"

// TODO: Fix the code so this hack can be removed.
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic ignored "-Wextended-offsetof"
#endif

namespace hadesmem
{

struct Export::Impl
{
  explicit Impl(Process const& process, PeFile const& pe_file, 
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
        HADESMEM_THROW_EXCEPTION(Error() << 
          ErrorString("Invalid forwarder string format."));
      }
    }
    else
    {
      rva_ = func_rva;
      va_ = RvaToVa(process, pe_file, func_rva);
    }
  }

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
  bool forwarded_by_ordinal_;
  WORD forwarder_ordinal_;
};

Export::Export(Process const& process, PeFile const& pe_file, WORD ordinal)
  : impl_(new Impl(process, pe_file, ordinal))
{ }

Export::Export(Export const& other)
  : impl_(new Impl(*other.impl_))
{ }

Export& Export::operator=(Export const& other)
{
  impl_ = std::unique_ptr<Impl>(new Impl(*other.impl_));

  return *this;
}

Export::Export(Export&& other) HADESMEM_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

Export& Export::operator=(Export&& other) HADESMEM_NOEXCEPT
{
  impl_ = std::move(other.impl_);
  
  return *this;
}

Export::~Export()
{ }

DWORD Export::GetRva() const
{
  return impl_->rva_;
}

PVOID Export::GetVa() const
{
  return impl_->va_;
}

std::string Export::GetName() const
{
  return impl_->name_;
}

std::string Export::GetForwarder() const
{
  return impl_->forwarder_;
}

std::string Export::GetForwarderModule() const
{
  return impl_->forwarder_split_.first;
}

std::string Export::GetForwarderFunction() const
{
  return impl_->forwarder_split_.second;
}

WORD Export::GetOrdinal() const
{
  return impl_->ordinal_;
}

bool Export::ByName() const
{
  return impl_->by_name_;
}

bool Export::ByOrdinal() const
{
  return !impl_->by_name_;
}

bool Export::IsForwarded() const
{
  return impl_->forwarded_;
}

bool Export::IsForwardedByOrdinal() const
{
  return (GetForwarderFunction()[0] == '#');
}

WORD Export::GetForwarderOrdinal() const
{
  if (!IsForwardedByOrdinal())
  {
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Function is not exported by ordinal."));
  }

  try
  {
    std::string const forwarder_function(GetForwarderFunction());
    return boost::lexical_cast<WORD>(forwarder_function.substr(1));
  }
  catch (std::exception const& /*e*/)
  {
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Invalid forwarder ordinal detected."));
  }
}

bool operator==(Export const& lhs, Export const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetOrdinal() == rhs.GetOrdinal();
}

bool operator!=(Export const& lhs, Export const& rhs) HADESMEM_NOEXCEPT
{
  return !(lhs == rhs);
}

bool operator<(Export const& lhs, Export const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetOrdinal() < rhs.GetOrdinal();
}

bool operator<=(Export const& lhs, Export const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetOrdinal() <= rhs.GetOrdinal();
}

bool operator>(Export const& lhs, Export const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetOrdinal() > rhs.GetOrdinal();
}

bool operator>=(Export const& lhs, Export const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetOrdinal() >= rhs.GetOrdinal();
}

std::ostream& operator<<(std::ostream& lhs, Export const& rhs)
{
  return (lhs << rhs.GetOrdinal());
}

std::wostream& operator<<(std::wostream& lhs, Export const& rhs)
{
  return (lhs << rhs.GetOrdinal());
}

}

// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/pelib/pe_file.hpp"

#include <cstddef>
#include <ostream>
#include <utility>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/assert.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include <windows.h>
#include <winnt.h>

#include "hadesmem/read.hpp"
#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/pelib/section.hpp"
#include "hadesmem/pelib/section_list.hpp"

namespace hadesmem
{

struct PeFile::Impl
{
  explicit Impl(Process const& process, PVOID address, PeFileType type) 
    HADESMEM_NOEXCEPT
    : process_(&process), 
    base_(static_cast<PBYTE>(address)), 
    type_(type)
  {
    BOOST_ASSERT(base_ != 0);
  }

  Process const* process_;
  PBYTE base_;
  PeFileType type_;
};

PeFile::PeFile(Process const& process, PVOID address, PeFileType type)
  : impl_(new Impl(process, address, type))
{ }

PeFile::PeFile(PeFile const& other)
  : impl_(new Impl(*other.impl_))
{ }

PeFile& PeFile::operator=(PeFile const& other)
{
  impl_ = std::unique_ptr<Impl>(new Impl(*other.impl_));

  return *this;
}

PeFile::PeFile(PeFile&& other) HADESMEM_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

PeFile& PeFile::operator=(PeFile&& other) HADESMEM_NOEXCEPT
{
  impl_ = std::move(other.impl_);
  
  return *this;
}

PeFile::~PeFile()
{ }

PVOID PeFile::GetBase() const HADESMEM_NOEXCEPT
{
  return impl_->base_;
}

PeFileType PeFile::GetType() const HADESMEM_NOEXCEPT
{
  return impl_->type_;
}

bool operator==(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

bool operator!=(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT
{
  return !(lhs == rhs);
}

bool operator<(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

bool operator<=(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

bool operator>(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

bool operator>=(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

std::ostream& operator<<(std::ostream& lhs, PeFile const& rhs)
{
  return (lhs << rhs.GetBase());
}

std::wostream& operator<<(std::wostream& lhs, PeFile const& rhs)
{
  return (lhs << rhs.GetBase());
}

PVOID RvaToVa(Process const& process, PeFile const& pe_file, DWORD rva)
{
  PeFileType const type = pe_file.GetType();
  PBYTE base = static_cast<PBYTE>(pe_file.GetBase());

  if (type == PeFileType::Data)
  {
    if (!rva)
    {
      return nullptr;
    }

    SectionList sections(process, pe_file);
    for (auto const& section : sections)
    {
      DWORD const virtual_beg = section.GetVirtualAddress();
      DWORD const virtual_end = virtual_beg + section.GetVirtualSize();
      if (virtual_beg <= rva && rva < virtual_end)
      {
        rva -= virtual_beg;
        // If PointerToRawData is less than 0x200 it is rounded down to 0.
        // TODO: Check if this is the correct way to be doing this. In the 
        // Windows loader this is probably done as part of a more complex 
        // mask, containing logic that's missing here.
        if (section.GetPointerToRawData() >= 0x200)
        {
          rva += section.GetPointerToRawData();
        }

        return base + rva;
      }
    }
    
    // Windows will load specially crafted images with no sections.
    // TODO: Check whether FileAlignment and/or SectionAlignment should be 
    // checked here. In the specially crafted image I'm testing this against 
    // the value is '1' for both anyway, but I'd like to ensure it's not 
    // possible for it to be higher, and if it is, whether it would affect 
    // the RVA resolution here.
    if (std::begin(sections) == std::end(sections))
    {
      return base + rva;
    }

    return nullptr;
  }
  else if (type == PeFileType::Image)
  {
    return rva ? (base + rva) : nullptr;
  }
  else
  {
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Unhandled file type."));
  }
}

}

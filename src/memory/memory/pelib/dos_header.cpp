// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/pelib/dos_header.hpp"

#include <cstddef>
#include <utility>
#include <iostream>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/assert.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include <windows.h>
#include <winnt.h>

#include "hadesmem/read.hpp"
#include "hadesmem/error.hpp"
#include "hadesmem/write.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/pelib/pe_file.hpp"

namespace hadesmem
{

struct DosHeader::Impl
{
  explicit Impl(Process const& process, PeFile const& pe_file) 
    HADESMEM_NOEXCEPT
    : process_(&process), 
    pe_file_(&pe_file), 
    base_(static_cast<PBYTE>(pe_file.GetBase()))
  { }

  Process const* process_;
  PeFile const* pe_file_;
  PBYTE base_;
};

DosHeader::DosHeader(Process const& process, PeFile const& pefile)
  : impl_(new Impl(process, pefile))
{
  EnsureValid();
}

DosHeader::DosHeader(DosHeader const& other)
  : impl_(new Impl(*other.impl_))
{ }

DosHeader& DosHeader::operator=(DosHeader const& other)
{
  impl_ = std::unique_ptr<Impl>(new Impl(*other.impl_));

  return *this;
}

DosHeader::DosHeader(DosHeader&& other) HADESMEM_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

DosHeader& DosHeader::operator=(DosHeader&& other) HADESMEM_NOEXCEPT
{
  impl_ = std::move(other.impl_);
  
  return *this;
}

DosHeader::~DosHeader()
{ }

PVOID DosHeader::GetBase() const HADESMEM_NOEXCEPT
{
  return impl_->base_;
}

bool DosHeader::IsValid() const
{
  return IMAGE_DOS_SIGNATURE == GetMagic();
}

void DosHeader::EnsureValid() const
{
  if (!IsValid())
  {
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("DOS header magic invalid."));
  }
}

WORD DosHeader::GetMagic() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, 
    e_magic));
}

WORD DosHeader::GetBytesOnLastPage() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, 
    e_cblp));
}

WORD DosHeader::GetPagesInFile() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, 
    e_cp));

}

WORD DosHeader::GetRelocations() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, 
    e_crlc));
}

WORD DosHeader::GetSizeOfHeaderInParagraphs() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, 
    e_cparhdr));
}

WORD DosHeader::GetMinExtraParagraphs() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, 
    e_minalloc));
}

WORD DosHeader::GetMaxExtraParagraphs() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, 
    e_maxalloc));
}

WORD DosHeader::GetInitialSS() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, 
    e_ss));
}

WORD DosHeader::GetInitialSP() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, 
    e_sp));
}

WORD DosHeader::GetChecksum() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, 
    e_csum));
}

WORD DosHeader::GetInitialIP() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, 
    e_ip));
}

WORD DosHeader::GetInitialCS() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, 
    e_cs));
}

WORD DosHeader::GetRelocTableFileAddr() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, 
    e_lfarlc));
}

WORD DosHeader::GetOverlayNum() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, 
    e_ovno));
}

std::array<WORD, 4> DosHeader::GetReservedWords1() const
{
  return Read<WORD, 4>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_DOS_HEADER, e_res));
}

WORD DosHeader::GetOEMID() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, 
    e_oemid));
}

WORD DosHeader::GetOEMInfo() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, 
    e_oeminfo));
}

std::array<WORD, 10> DosHeader::GetReservedWords2() const
{
  return Read<WORD, 10>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_DOS_HEADER, e_res2));
}

LONG DosHeader::GetNewHeaderOffset() const
{
  return Read<LONG>(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, 
    e_lfanew));
}

void DosHeader::SetMagic(WORD magic) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, e_magic), 
    magic);
}

void DosHeader::SetBytesOnLastPage(WORD bytes_on_last_page) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, e_cblp), 
    bytes_on_last_page);
}

void DosHeader::SetPagesInFile(WORD pages_in_file) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, e_cp), 
    pages_in_file);
}

void DosHeader::SetRelocations(WORD relocations) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, e_crlc), 
    relocations);
}

void DosHeader::SetSizeOfHeaderInParagraphs(WORD size_of_header_in_paragraphs) 
  const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, e_cparhdr), 
    size_of_header_in_paragraphs);
}

void DosHeader::SetMinExtraParagraphs(WORD min_extra_paragraphs) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, e_minalloc), 
    min_extra_paragraphs);
}

void DosHeader::SetMaxExtraParagraphs(WORD max_extra_paragraphs) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, 
    e_maxalloc), max_extra_paragraphs);
}

void DosHeader::SetInitialSS(WORD initial_ss) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, e_ss), 
    initial_ss);
}

void DosHeader::SetInitialSP(WORD initial_sp) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, e_sp), 
    initial_sp);
}

void DosHeader::SetChecksum(WORD checksum) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, e_csum), 
    checksum);
}

void DosHeader::SetInitialIP(WORD initial_ip) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, e_ip), 
    initial_ip);
}

void DosHeader::SetInitialCS(WORD initial_cs) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, e_cs), 
    initial_cs);
}

void DosHeader::SetRelocTableFileAddr(WORD reloc_table_file_addr) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, e_lfarlc), 
    reloc_table_file_addr);
}

void DosHeader::SetOverlayNum(WORD overlay_num) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, e_ovno), 
    overlay_num);
}

void DosHeader::SetReservedWords1(
  std::array<WORD, 4> const& reserved_words_1) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, e_res), 
    reserved_words_1);
}

void DosHeader::SetOEMID(WORD oem_id) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, e_oemid), 
    oem_id);
}

void DosHeader::SetOEMInfo(WORD oem_info) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, e_oeminfo), 
    oem_info);
}

void DosHeader::SetReservedWords2(
  std::array<WORD, 10> const& reserved_words_2) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, e_res2), 
    reserved_words_2);
}

void DosHeader::SetNewHeaderOffset(LONG offset) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_DOS_HEADER, e_lfanew), 
    offset);
}

bool operator==(DosHeader const& lhs, DosHeader const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

bool operator!=(DosHeader const& lhs, DosHeader const& rhs) HADESMEM_NOEXCEPT
{
  return !(lhs == rhs);
}

bool operator<(DosHeader const& lhs, DosHeader const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

bool operator<=(DosHeader const& lhs, DosHeader const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

bool operator>(DosHeader const& lhs, DosHeader const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

bool operator>=(DosHeader const& lhs, DosHeader const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

std::ostream& operator<<(std::ostream& lhs, DosHeader const& rhs)
{
  return (lhs << rhs.GetBase());
}

std::wostream& operator<<(std::wostream& lhs, DosHeader const& rhs)
{
  return (lhs << rhs.GetBase());
}

}

// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <array>
#include <iosfwd>
#include <memory>

#include <windows.h>

#include "hadesmem/config.hpp"

namespace hadesmem
{

class Process;

class PeFile;

class DosHeader
{
public:
  explicit DosHeader(Process const& process, PeFile const& pe_file);

  DosHeader(DosHeader const& other);
  
  DosHeader& operator=(DosHeader const& other);

  DosHeader(DosHeader&& other) HADESMEM_NOEXCEPT;
  
  DosHeader& operator=(DosHeader&& other) HADESMEM_NOEXCEPT;
  
  ~DosHeader();

  PVOID GetBase() const HADESMEM_NOEXCEPT;
  
  bool IsValid() const;
  
  void EnsureValid() const;
  
  WORD GetMagic() const;
  
  WORD GetBytesOnLastPage() const;
  
  WORD GetPagesInFile() const;
  
  WORD GetRelocations() const;
  
  WORD GetSizeOfHeaderInParagraphs() const;
  
  WORD GetMinExtraParagraphs() const;
  
  WORD GetMaxExtraParagraphs() const;
  
  WORD GetInitialSS() const;
  
  WORD GetInitialSP() const;
  
  WORD GetChecksum() const;
  
  WORD GetInitialIP() const;
  
  WORD GetInitialCS() const;
  
  WORD GetRelocTableFileAddr() const;
  
  WORD GetOverlayNum() const;
  
  std::array<WORD, 4> GetReservedWords1() const;
  
  WORD GetOEMID() const;
  
  WORD GetOEMInfo() const;
  
  std::array<WORD, 10> GetReservedWords2() const;
  
  LONG GetNewHeaderOffset() const;
  
  void SetMagic(WORD magic) const;
  
  void SetBytesOnLastPage(WORD bytes_on_last_page) const;
  
  void SetPagesInFile(WORD pages_in_file) const;
  
  void SetRelocations(WORD relocations) const;
  
  void SetSizeOfHeaderInParagraphs(WORD size_of_header_in_paragraphs) const;
  
  void SetMinExtraParagraphs(WORD min_extra_paragraphs) const;
  
  void SetMaxExtraParagraphs(WORD max_extra_paragraphs) const;
  
  void SetInitialSS(WORD initial_ss) const;
  
  void SetInitialSP(WORD initial_sp) const;
  
  void SetChecksum(WORD checksum) const;
  
  void SetInitialIP(WORD initial_ip) const;
  
  void SetInitialCS(WORD initial_cs) const;
  
  void SetRelocTableFileAddr(WORD reloc_table_file_addr) const;
  
  void SetOverlayNum(WORD overlay_num) const;
  
  void SetReservedWords1(std::array<WORD, 4> const& reserved_words_1) const;
  
  void SetOEMID(WORD oem_id) const;
  
  void SetOEMInfo(WORD oem_info) const;
  
  void SetReservedWords2(std::array<WORD, 10> const& reserved_words_2) const;

  void SetNewHeaderOffset(LONG offset) const;
  
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

bool operator==(DosHeader const& lhs, DosHeader const& rhs) HADESMEM_NOEXCEPT;

bool operator!=(DosHeader const& lhs, DosHeader const& rhs) HADESMEM_NOEXCEPT;

bool operator<(DosHeader const& lhs, DosHeader const& rhs) HADESMEM_NOEXCEPT;

bool operator<=(DosHeader const& lhs, DosHeader const& rhs) HADESMEM_NOEXCEPT;

bool operator>(DosHeader const& lhs, DosHeader const& rhs) HADESMEM_NOEXCEPT;

bool operator>=(DosHeader const& lhs, DosHeader const& rhs) HADESMEM_NOEXCEPT;

std::ostream& operator<<(std::ostream& lhs, DosHeader const& rhs);

std::wostream& operator<<(std::wostream& lhs, DosHeader const& rhs);

}

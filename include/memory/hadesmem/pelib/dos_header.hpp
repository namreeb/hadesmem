// Copyright (C) 2010-2013 Joshua Boyce.
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
  
  void SetMagic(WORD magic);
  
  void SetBytesOnLastPage(WORD bytes_on_last_page);
  
  void SetPagesInFile(WORD pages_in_file);
  
  void SetRelocations(WORD relocations);
  
  void SetSizeOfHeaderInParagraphs(WORD size_of_header_in_paragraphs);
  
  void SetMinExtraParagraphs(WORD min_extra_paragraphs);
  
  void SetMaxExtraParagraphs(WORD max_extra_paragraphs);
  
  void SetInitialSS(WORD initial_ss);
  
  void SetInitialSP(WORD initial_sp);
  
  void SetChecksum(WORD checksum);
  
  void SetInitialIP(WORD initial_ip);
  
  void SetInitialCS(WORD initial_cs);
  
  void SetRelocTableFileAddr(WORD reloc_table_file_addr);
  
  void SetOverlayNum(WORD overlay_num);
  
  void SetReservedWords1(std::array<WORD, 4> const& reserved_words_1);
  
  void SetOEMID(WORD oem_id);
  
  void SetOEMInfo(WORD oem_info);
  
  void SetReservedWords2(std::array<WORD, 10> const& reserved_words_2);

  void SetNewHeaderOffset(LONG offset);
  
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

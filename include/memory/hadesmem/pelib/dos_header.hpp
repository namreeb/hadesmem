// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <array>
#include <iosfwd>
#include <memory>
#include <cstddef>
#include <ostream>
#include <utility>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/assert.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <windows.h>
#include <winnt.h>

#include <hadesmem/read.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/write.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/pelib/pe_file.hpp>

namespace hadesmem
{

class DosHeader
{
public:
  explicit DosHeader(Process const& process, PeFile const& pe_file) 
    HADESMEM_NOEXCEPT
    : process_(&process), 
    pe_file_(&pe_file), 
    base_(static_cast<PBYTE>(pe_file.GetBase()))
  {
    EnsureValid();
  }

  DosHeader(DosHeader const& other) HADESMEM_NOEXCEPT
    : process_(other.process_), 
    pe_file_(other.pe_file_), 
    base_(other.base_)
  { }
  
  DosHeader& operator=(DosHeader const& other) HADESMEM_NOEXCEPT
  {
    process_ = other.process_;
    pe_file_ = other.pe_file_;
    base_ = other.base_;

    return *this;
  }

  DosHeader(DosHeader&& other) HADESMEM_NOEXCEPT
    : process_(other.process_), 
    pe_file_(other.pe_file_), 
    base_(other.base_)
  { }
  
  DosHeader& operator=(DosHeader&& other) HADESMEM_NOEXCEPT
  {
    process_ = other.process_;
    pe_file_ = other.pe_file_;
    base_ = other.base_;

    return *this;
  }
  
  ~DosHeader() HADESMEM_NOEXCEPT
  { }

  PVOID GetBase() const HADESMEM_NOEXCEPT
  {
    return base_;
  }
  
  bool IsValid() const
  {
    return IMAGE_DOS_SIGNATURE == GetMagic();
  }
  
  void EnsureValid() const
  {
    if (!IsValid())
    {
      HADESMEM_THROW_EXCEPTION(Error() << 
        ErrorString("DOS header magic invalid."));
    }
  }
  
  WORD GetMagic() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_DOS_HEADER, e_magic));
  }
  
  WORD GetBytesOnLastPage() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_DOS_HEADER, e_cblp));
  }
  
  WORD GetPagesInFile() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_DOS_HEADER, e_cp));
  }
  
  WORD GetRelocations() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_DOS_HEADER, e_crlc));
  }
  
  WORD GetSizeOfHeaderInParagraphs() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_DOS_HEADER, 
      e_cparhdr));
  }
  
  WORD GetMinExtraParagraphs() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_DOS_HEADER, 
      e_minalloc));
  }
  
  WORD GetMaxExtraParagraphs() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_DOS_HEADER, 
      e_maxalloc));
  }
  
  WORD GetInitialSS() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_DOS_HEADER, e_ss));
  }
  
  WORD GetInitialSP() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_DOS_HEADER, e_sp));
  }
  
  WORD GetChecksum() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_DOS_HEADER, e_csum));
  }
  
  WORD GetInitialIP() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_DOS_HEADER, e_ip));
  }
  
  WORD GetInitialCS() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_DOS_HEADER, e_cs));
  }
  
  WORD GetRelocTableFileAddr() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_DOS_HEADER, 
      e_lfarlc));
  }
  
  WORD GetOverlayNum() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_DOS_HEADER, e_ovno));
  }
  
  std::array<WORD, 4> GetReservedWords1() const
  {
    return Read<WORD, 4>(*process_, base_ + offsetof(IMAGE_DOS_HEADER, 
      e_res));
  }
  
  WORD GetOEMID() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_DOS_HEADER, e_oemid));
  }
  
  WORD GetOEMInfo() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_DOS_HEADER, 
      e_oeminfo));
  }
  
  std::array<WORD, 10> GetReservedWords2() const
  {
    return Read<WORD, 10>(*process_, base_ + offsetof(IMAGE_DOS_HEADER, 
      e_res2));
  }
  
  LONG GetNewHeaderOffset() const
  {
    return Read<LONG>(*process_, base_ + offsetof(IMAGE_DOS_HEADER, 
      e_lfanew));
  }
  
  void SetMagic(WORD magic)
  {
    Write(*process_, base_ + offsetof(IMAGE_DOS_HEADER, e_magic), magic);
  }
  
  void SetBytesOnLastPage(WORD bytes_on_last_page)
  {
    Write(*process_, base_ + offsetof(IMAGE_DOS_HEADER, e_cblp), 
      bytes_on_last_page);
  }
  
  void SetPagesInFile(WORD pages_in_file)
  {
    Write(*process_, base_ + offsetof(IMAGE_DOS_HEADER, e_cp), pages_in_file);
  }
  
  void SetRelocations(WORD relocations)
  {
    Write(*process_, base_ + offsetof(IMAGE_DOS_HEADER, e_crlc), relocations);
  }
  
  void SetSizeOfHeaderInParagraphs(WORD size_of_header_in_paragraphs)
  {
    Write(*process_, base_ + offsetof(IMAGE_DOS_HEADER, e_cparhdr), 
      size_of_header_in_paragraphs);
  }
  
  void SetMinExtraParagraphs(WORD min_extra_paragraphs)
  {
    Write(*process_, base_ + offsetof(IMAGE_DOS_HEADER, e_minalloc), 
      min_extra_paragraphs);
  }
  
  void SetMaxExtraParagraphs(WORD max_extra_paragraphs)
  {
    Write(*process_, base_ + offsetof(IMAGE_DOS_HEADER, e_maxalloc), 
      max_extra_paragraphs);
  }
  
  void SetInitialSS(WORD initial_ss)
  {
    Write(*process_, base_ + offsetof(IMAGE_DOS_HEADER, e_ss), initial_ss);
  }
  
  void SetInitialSP(WORD initial_sp)
  {
    Write(*process_, base_ + offsetof(IMAGE_DOS_HEADER, e_sp), initial_sp);
  }
  
  void SetChecksum(WORD checksum)
  {
    Write(*process_, base_ + offsetof(IMAGE_DOS_HEADER, e_csum), checksum);
  }
  
  void SetInitialIP(WORD initial_ip)
  {
    Write(*process_, base_ + offsetof(IMAGE_DOS_HEADER, e_ip), initial_ip);
  }
  
  void SetInitialCS(WORD initial_cs)
  {
    Write(*process_, base_ + offsetof(IMAGE_DOS_HEADER, e_cs), initial_cs);
  }
  
  void SetRelocTableFileAddr(WORD reloc_table_file_addr)
  {
    Write(*process_, base_ + offsetof(IMAGE_DOS_HEADER, e_lfarlc), 
      reloc_table_file_addr);
  }
  
  void SetOverlayNum(WORD overlay_num)
  {
    Write(*process_, base_ + offsetof(IMAGE_DOS_HEADER, e_ovno), overlay_num);
  }
  
  void SetReservedWords1(std::array<WORD, 4> const& reserved_words_1)
  {
    Write(*process_, base_ + offsetof(IMAGE_DOS_HEADER, e_res), 
      reserved_words_1);
  }
  
  void SetOEMID(WORD oem_id)
  {
    Write(*process_, base_ + offsetof(IMAGE_DOS_HEADER, e_oemid), oem_id);
  }
  
  void SetOEMInfo(WORD oem_info)
  {
    Write(*process_, base_ + offsetof(IMAGE_DOS_HEADER, e_oeminfo), oem_info);
  }
  
  void SetReservedWords2(std::array<WORD, 10> const& reserved_words_2)
  {
    Write(*process_, base_ + offsetof(IMAGE_DOS_HEADER, e_res2), 
      reserved_words_2);
  }

  void SetNewHeaderOffset(LONG offset)
  {
    Write(*process_, base_ + offsetof(IMAGE_DOS_HEADER, e_lfanew), offset);
  }
  
private:
  Process const* process_;
  PeFile const* pe_file_;
  PBYTE base_;
};

inline bool operator==(DosHeader const& lhs, DosHeader const& rhs) 
  HADESMEM_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

inline bool operator!=(DosHeader const& lhs, DosHeader const& rhs) 
  HADESMEM_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(DosHeader const& lhs, DosHeader const& rhs) 
  HADESMEM_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

inline bool operator<=(DosHeader const& lhs, DosHeader const& rhs) 
  HADESMEM_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

inline bool operator>(DosHeader const& lhs, DosHeader const& rhs) 
  HADESMEM_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

inline bool operator>=(DosHeader const& lhs, DosHeader const& rhs) 
  HADESMEM_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

inline std::ostream& operator<<(std::ostream& lhs, DosHeader const& rhs)
{
  return (lhs << rhs.GetBase());
}

inline std::wostream& operator<<(std::wostream& lhs, DosHeader const& rhs)
{
  return (lhs << rhs.GetBase());
}

}

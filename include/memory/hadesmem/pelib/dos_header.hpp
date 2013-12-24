// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <array>
#include <cstddef>
#include <iosfwd>
#include <memory>
#include <ostream>
#include <utility>

#include <windows.h>
#include <winnt.h>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

namespace hadesmem
{

class DosHeader
{
public:
  explicit DosHeader(Process const& process,
                     PeFile const& pe_file) HADESMEM_DETAIL_NOEXCEPT
    : process_(&process),
      base_(static_cast<PBYTE>(pe_file.GetBase())),
      data_()
  {
    UpdateRead();

    EnsureValid();
  }

  PVOID GetBase() const HADESMEM_DETAIL_NOEXCEPT
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
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error() << ErrorString("DOS header magic invalid."));
    }
  }

  void UpdateRead()
  {
    data_ = Read<IMAGE_DOS_HEADER>(*process_, base_);
  }

  void UpdateWrite()
  {
    Write(*process_, base_, data_);
  }

  WORD GetMagic() const
  {
    return data_.e_magic;
  }

  WORD GetBytesOnLastPage() const
  {
    return data_.e_cblp;
  }

  WORD GetPagesInFile() const
  {
    return data_.e_cp;
  }

  WORD GetRelocations() const
  {
    return data_.e_crlc;
  }

  WORD GetSizeOfHeaderInParagraphs() const
  {
    return data_.e_cparhdr;
  }

  WORD GetMinExtraParagraphs() const
  {
    return data_.e_minalloc;
  }

  WORD GetMaxExtraParagraphs() const
  {
    return data_.e_maxalloc;
  }

  WORD GetInitialSS() const
  {
    return data_.e_ss;
  }

  WORD GetInitialSP() const
  {
    return data_.e_sp;
  }

  WORD GetChecksum() const
  {
    return data_.e_csum;
  }

  WORD GetInitialIP() const
  {
    return data_.e_ip;
  }

  WORD GetInitialCS() const
  {
    return data_.e_cs;
  }

  WORD GetRelocTableFileAddr() const
  {
    return data_.e_lfarlc;
  }

  WORD GetOverlayNum() const
  {
    return data_.e_ovno;
  }

  std::array<WORD, 4> GetReservedWords1() const
  {
    std::array<WORD, 4> out;
    std::copy(std::begin(data_.e_res), std::end(data_.e_res), std::begin(out));
    return out;
  }

  WORD GetOEMID() const
  {
    return data_.e_oemid;
  }

  WORD GetOEMInfo() const
  {
    return data_.e_oeminfo;
  }

  std::array<WORD, 10> GetReservedWords2() const
  {
    std::array<WORD, 10> out;
    std::copy(
      std::begin(data_.e_res2), std::end(data_.e_res2), std::begin(out));
    return out;
  }

  LONG GetNewHeaderOffset() const
  {
    return data_.e_lfanew;
  }

  void SetMagic(WORD magic)
  {
    data_.e_magic = magic;
  }

  void SetBytesOnLastPage(WORD bytes_on_last_page)
  {
    data_.e_cblp = bytes_on_last_page;
  }

  void SetPagesInFile(WORD pages_in_file)
  {
    data_.e_cp = pages_in_file;
  }

  void SetRelocations(WORD relocations)
  {
    data_.e_crlc = relocations;
  }

  void SetSizeOfHeaderInParagraphs(WORD size_of_header_in_paragraphs)
  {
    data_.e_cparhdr = size_of_header_in_paragraphs;
  }

  void SetMinExtraParagraphs(WORD min_extra_paragraphs)
  {
    data_.e_minalloc = min_extra_paragraphs;
  }

  void SetMaxExtraParagraphs(WORD max_extra_paragraphs)
  {
    data_.e_maxalloc = max_extra_paragraphs;
  }

  void SetInitialSS(WORD initial_ss)
  {
    data_.e_ss = initial_ss;
  }

  void SetInitialSP(WORD initial_sp)
  {
    data_.e_sp = initial_sp;
  }

  void SetChecksum(WORD checksum)
  {
    data_.e_csum = checksum;
  }

  void SetInitialIP(WORD initial_ip)
  {
    data_.e_ip = initial_ip;
  }

  void SetInitialCS(WORD initial_cs)
  {
    data_.e_cs = initial_cs;
  }

  void SetRelocTableFileAddr(WORD reloc_table_file_addr)
  {
    data_.e_lfarlc = reloc_table_file_addr;
  }

  void SetOverlayNum(WORD overlay_num)
  {
    data_.e_ovno = overlay_num;
  }

  void SetReservedWords1(std::array<WORD, 4> const& reserved_words_1)
  {
    std::copy(std::begin(reserved_words_1),
              std::end(reserved_words_1),
              std::begin(data_.e_res));
  }

  void SetOEMID(WORD oem_id)
  {
    data_.e_oemid = oem_id;
  }

  void SetOEMInfo(WORD oem_info)
  {
    data_.e_oeminfo = oem_info;
  }

  void SetReservedWords2(std::array<WORD, 10> const& reserved_words_2)
  {
    std::copy(std::begin(reserved_words_2),
              std::end(reserved_words_2),
              std::begin(data_.e_res2));
  }

  void SetNewHeaderOffset(LONG offset)
  {
    data_.e_lfanew = offset;
  }

private:
  Process const* process_;
  PBYTE base_;
  IMAGE_DOS_HEADER data_;
};

inline bool operator==(DosHeader const& lhs, DosHeader const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

inline bool operator!=(DosHeader const& lhs, DosHeader const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(DosHeader const& lhs, DosHeader const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

inline bool operator<=(DosHeader const& lhs, DosHeader const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

inline bool operator>(DosHeader const& lhs, DosHeader const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

inline bool operator>=(DosHeader const& lhs, DosHeader const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

inline std::ostream& operator<<(std::ostream& lhs, DosHeader const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << static_cast<void*>(rhs.GetBase());
  lhs.imbue(old);
  return lhs;
}

inline std::wostream& operator<<(std::wostream& lhs, DosHeader const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << static_cast<void*>(rhs.GetBase());
  lhs.imbue(old);
  return lhs;
}
}

// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstddef>
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

// TODO: Add tests.

namespace hadesmem
{

class Relocation
{
public:
  Relocation(Process const& process, PeFile const& pe_file, PWORD base)
    : process_(&process),
      pe_file_(&pe_file),
      base_(reinterpret_cast<PBYTE>(base)),
      type_(0),
      offset_(0)
  {
    UpdateRead();
  }

  PVOID GetBase() const HADESMEM_DETAIL_NOEXCEPT
  {
    return base_;
  }

  void UpdateRead()
  {
    auto const data_tmp = Read<WORD>(*process_, base_);
    type_ = static_cast<BYTE>(data_tmp >> 12);
    offset_ = data_tmp & 0x0FFF;
  }

  void UpdateWrite()
  {
    WORD const data_tmp = (offset_ | (type_ << 12));
    Write(*process_, base_, data_tmp);
  }

  BYTE GetType() const HADESMEM_DETAIL_NOEXCEPT
  {
    return type_;
  }

  void SetType(BYTE type) HADESMEM_DETAIL_NOEXCEPT
  {
    type_ = type;
  }

  WORD GetOffset() const HADESMEM_DETAIL_NOEXCEPT
  {
    return offset_;
  }

  void SetOffset(WORD offset) HADESMEM_DETAIL_NOEXCEPT
  {
    offset_ = offset;
  }

private:
  Process const* process_;
  PeFile const* pe_file_;
  PBYTE base_;
  BYTE type_;
  WORD offset_;
};

inline bool operator==(Relocation const& lhs, Relocation const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

inline bool operator!=(Relocation const& lhs, Relocation const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(Relocation const& lhs, Relocation const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

inline bool operator<=(Relocation const& lhs, Relocation const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

inline bool operator>(Relocation const& lhs, Relocation const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

inline bool operator>=(Relocation const& lhs, Relocation const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

inline std::ostream& operator<<(std::ostream& lhs, Relocation const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << rhs.GetBase();
  lhs.imbue(old);
  return lhs;
}

inline std::wostream& operator<<(std::wostream& lhs, Relocation const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << rhs.GetBase();
  lhs.imbue(old);
  return lhs;
}
}

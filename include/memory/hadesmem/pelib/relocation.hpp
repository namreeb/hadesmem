// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstddef>
#include <cstdint>
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

class Relocation
{
public:
  explicit Relocation(Process const& process,
                      PeFile const& pe_file,
                      std::uint16_t* base)
    : process_{&process},
      pe_file_{&pe_file},
      base_{reinterpret_cast<std::uint8_t*>(base)}
  {
    UpdateRead();
  }

  explicit Relocation(Process&& process,
                      PeFile const& pe_file,
                      std::uint16_t* base) = delete;

  explicit Relocation(Process const& process,
                      PeFile&& pe_file,
                      std::uint16_t* base) = delete;

  explicit Relocation(Process&& process,
                      PeFile&& pe_file,
                      std::uint16_t* base) = delete;

  void* GetBase() const HADESMEM_DETAIL_NOEXCEPT
  {
    return base_;
  }

  void UpdateRead()
  {
    auto const data_tmp = Read<std::uint16_t>(*process_, base_);
    type_ = static_cast<std::uint8_t>(data_tmp >> 12);
    offset_ = data_tmp & 0x0FFF;
  }

  void UpdateWrite()
  {
    auto const data_tmp =
      static_cast<std::uint16_t>(static_cast<std::uint32_t>(offset_) |
                                 (static_cast<std::uint32_t>(type_) << 12));
    Write(*process_, base_, data_tmp);
  }

  std::uint8_t GetType() const HADESMEM_DETAIL_NOEXCEPT
  {
    return type_;
  }

  void SetType(std::uint8_t type) HADESMEM_DETAIL_NOEXCEPT
  {
    type_ = type;
  }

  std::uint16_t GetOffset() const HADESMEM_DETAIL_NOEXCEPT
  {
    return offset_;
  }

  void SetOffset(std::uint16_t offset) HADESMEM_DETAIL_NOEXCEPT
  {
    offset_ = offset;
  }

private:
  Process const* process_;
  PeFile const* pe_file_;
  std::uint8_t* base_;
  std::uint8_t type_{};
  std::uint16_t offset_{};
};

inline bool operator==(Relocation const& lhs,
                       Relocation const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

inline bool operator!=(Relocation const& lhs,
                       Relocation const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(Relocation const& lhs,
                      Relocation const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

inline bool operator<=(Relocation const& lhs,
                       Relocation const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

inline bool operator>(Relocation const& lhs,
                      Relocation const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

inline bool operator>=(Relocation const& lhs,
                       Relocation const& rhs) HADESMEM_DETAIL_NOEXCEPT
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

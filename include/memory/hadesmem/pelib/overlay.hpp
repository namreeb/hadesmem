// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <ostream>
#include <string>
#include <utility>

#include <windows.h>
#include <winnt.h>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/pelib/nt_headers.hpp>
#include <hadesmem/pelib/section.hpp>
#include <hadesmem/pelib/section_list.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

namespace hadesmem
{
class Overlay
{
public:
  explicit Overlay(Process const& process, PeFile const& pe_file)
    : process_{&process}, pe_file_{&pe_file}, base_{}, size_{}
  {
    if (pe_file.GetType() != PeFileType::kData)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                      << ErrorString{"Invalid PE file type."});
    }

    hadesmem::NtHeaders const nt_headers{process, pe_file};
    auto const file_align = nt_headers.GetFileAlignment();

    DWORD overlay_offset = 0;

    hadesmem::SectionList const sections(process, pe_file);
    for (auto const& s : sections)
    {
      // http://bit.ly/1TFFkeT
      // TODO: Ensure this is correct.
      // TOOD: Investigate whether or not we're getting this sort of logic right
      // everywhere else.
      auto const pointer_to_raw = s.GetPointerToRawData();
      auto const aligned_pointer_to_raw = pointer_to_raw & ~0x1FF;
      auto const size_of_raw = s.GetSizeOfRawData();
      auto read_size = (((pointer_to_raw + size_of_raw) + file_align - 1) &
                        ~(file_align - 1)) -
                       aligned_pointer_to_raw;
      read_size = (std::min)(read_size, (size_of_raw + 0xFFF) & ~0xFFF);
      if (auto const virtual_size = s.GetVirtualSize())
      {
        read_size = (std::min)(read_size, (virtual_size + 0xFFF) & ~0xFFF);
      }

      auto const section_end = aligned_pointer_to_raw + read_size;
      if (section_end > overlay_offset)
      {
        overlay_offset = section_end;
      }
    }

    if (!overlay_offset || overlay_offset > pe_file.GetSize())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error{} << ErrorString{"Invalid overlay offset."});
    }

    if (overlay_offset == pe_file.GetSize())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error{} << ErrorString{"No overlay."});
    }

    size_ = pe_file.GetSize() - overlay_offset;
    base_ = static_cast<std::uint8_t*>(pe_file.GetBase()) + overlay_offset;

    UpdateRead();
  }

  explicit Overlay(Process const&& process, PeFile const& pe_file, void* base);

  explicit Overlay(Process const& process, PeFile&& pe_file, void* base);

  explicit Overlay(Process const&& process, PeFile&& pe_file, void* base);

  void* GetBase() const noexcept
  {
    return base_;
  }

  std::size_t GetSize() const noexcept
  {
    return data_.size();
  }

  std::uintptr_t GetOffset() const noexcept
  {
    return reinterpret_cast<std::uintptr_t>(base_) -
           reinterpret_cast<std::uintptr_t>(pe_file_->GetBase());
  }

  void UpdateRead()
  {
    data_ = ReadVector<std::uint8_t>(*process_, base_, size_);
  }

  void UpdateWrite()
  {
    WriteVector(*process_, base_, data_);
  }

  std::vector<std::uint8_t> Get() const
  {
    return data_;
  }

  void Set(std::vector<std::uint8_t> const& data)
  {
    // TODO: Support this. Especially important when crafting new files from
    // scratch or rebuilding packed files.
    if (data.size() != data_.size())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error{} << ErrorString{
          "Modifying overlay size currently unsupported."});
    }

    data_ = data;
  }

private:
  Process const* process_;
  PeFile const* pe_file_;
  std::uint8_t* base_;
  DWORD size_;
  std::vector<std::uint8_t> data_;
};

inline bool operator==(Overlay const& lhs, Overlay const& rhs) noexcept
{
  return lhs.GetBase() == rhs.GetBase();
}

inline bool operator!=(Overlay const& lhs, Overlay const& rhs) noexcept
{
  return !(lhs == rhs);
}

inline bool operator<(Overlay const& lhs, Overlay const& rhs) noexcept
{
  return lhs.GetBase() < rhs.GetBase();
}

inline bool operator<=(Overlay const& lhs, Overlay const& rhs) noexcept
{
  return lhs.GetBase() <= rhs.GetBase();
}

inline bool operator>(Overlay const& lhs, Overlay const& rhs) noexcept
{
  return lhs.GetBase() > rhs.GetBase();
}

inline bool operator>=(Overlay const& lhs, Overlay const& rhs) noexcept
{
  return lhs.GetBase() >= rhs.GetBase();
}

inline std::ostream& operator<<(std::ostream& lhs, Overlay const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << rhs.GetBase();
  lhs.imbue(old);
  return lhs;
}

inline std::wostream& operator<<(std::wostream& lhs, Overlay const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << rhs.GetBase();
  lhs.imbue(old);
  return lhs;
}
}

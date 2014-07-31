// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include <windows.h>

#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/protect_guard.hpp>
#include <hadesmem/detail/query_region.hpp>
#include <hadesmem/detail/type_traits.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/protect.hpp>

namespace hadesmem
{

struct ReadFlags
{
  enum : std::uint32_t
  { kNone,
    kZeroFillReserved };
};

namespace detail
{

inline void ReadUnchecked(Process const& process,
                          void* address,
                          void* data,
                          std::size_t len,
                          std::uint32_t /*flags*/ = ReadFlags::kNone)
{
  HADESMEM_DETAIL_ASSERT(len ? address != nullptr : true);
  HADESMEM_DETAIL_ASSERT(data != nullptr);

  if (!len)
  {
    return;
  }

  SIZE_T bytes_read = 0;
  if (!::ReadProcessMemory(
        process.GetHandle(), address, data, len, &bytes_read) ||
      bytes_read != len)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"ReadProcessMemory failed."}
                                    << ErrorCodeWinLast{last_error});
  }
}

inline void ReadImpl(Process const& process,
                     void* address,
                     void* data,
                     std::size_t len,
                     std::uint32_t flags = ReadFlags::kNone)
{
  HADESMEM_DETAIL_ASSERT(len ? address != nullptr : true);
  HADESMEM_DETAIL_ASSERT(data != nullptr);

  if (!len)
  {
    return;
  }

  for (;;)
  {
    MEMORY_BASIC_INFORMATION const mbi = detail::Query(process, address);

    void* const address_end = static_cast<std::uint8_t*>(address) + len;
    void* const region_next =
      static_cast<std::uint8_t*>(mbi.BaseAddress) + mbi.RegionSize;

    bool const should_zero_fill =
      (mbi.State == MEM_RESERVE && !!(flags & ReadFlags::kZeroFillReserved));

    if (address_end <= region_next)
    {
      if (should_zero_fill)
      {
        std::fill(static_cast<std::uint8_t*>(data),
                  static_cast<std::uint8_t*>(data) + len,
                  0);
      }
      else
      {
        ProtectGuard protect_guard{process, mbi, ProtectGuardType::kRead};
        ReadUnchecked(process, address, data, len, flags);
        protect_guard.Restore();
      }

      return;
    }
    else
    {
      std::size_t const len_new =
        reinterpret_cast<std::uintptr_t>(region_next) -
        reinterpret_cast<std::uintptr_t>(address);

      if (should_zero_fill)
      {
        std::fill(static_cast<std::uint8_t*>(data),
                  static_cast<std::uint8_t*>(data) + len_new,
                  0);
      }
      else
      {
        ProtectGuard protect_guard{process, mbi, ProtectGuardType::kRead};
        ReadUnchecked(process, address, data, len_new, flags);
        protect_guard.Restore();
      }

      address = static_cast<std::uint8_t*>(address) + len_new;
      data = static_cast<std::uint8_t*>(data) + len_new;
      len -= len_new;
    }
  }
}

template <typename T>
T ReadUnsafeImpl(Process const& process,
                 void* address,
                 std::uint32_t flags = ReadFlags::kNone)
{
  HADESMEM_DETAIL_STATIC_ASSERT(std::is_default_constructible<T>::value);

  HADESMEM_DETAIL_ASSERT(address != nullptr);

  T data;
  ReadImpl(process, address, std::addressof(data), sizeof(data), flags);
  return data;
}

template <typename T>
T ReadImpl(Process const& process,
           void* address,
           std::uint32_t flags = ReadFlags::kNone)
{
  HADESMEM_DETAIL_STATIC_ASSERT(detail::IsTriviallyCopyable<T>::value);
  HADESMEM_DETAIL_STATIC_ASSERT(std::is_default_constructible<T>::value);

  HADESMEM_DETAIL_ASSERT(address != nullptr);

  T data;
  ReadImpl(process, address, std::addressof(data), sizeof(data), flags);
  return data;
}
}
}

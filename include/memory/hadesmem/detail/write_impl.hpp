// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstddef>

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
namespace detail
{
inline void WriteUnchecked(Process const& process,
                           PVOID address,
                           LPCVOID data,
                           std::size_t len)
{
  HADESMEM_DETAIL_ASSERT(address != nullptr);
  HADESMEM_DETAIL_ASSERT(data != nullptr);
  HADESMEM_DETAIL_ASSERT(len != 0);

  SIZE_T bytes_written = 0;
  if (!::WriteProcessMemory(
        process.GetHandle(), address, data, len, &bytes_written) ||
      bytes_written != len)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"WriteProcessMemory failed."}
                                    << ErrorCodeWinLast{last_error});
  }
}

inline void WriteImpl(Process const& process,
                      PVOID address,
                      LPCVOID data,
                      std::size_t len)
{
  HADESMEM_DETAIL_ASSERT(address != nullptr);
  HADESMEM_DETAIL_ASSERT(data != nullptr);
  HADESMEM_DETAIL_ASSERT(len != 0);

  for (;;)
  {
    ProtectGuard protect_guard{process, address, ProtectGuardType::kWrite};

    MEMORY_BASIC_INFORMATION const mbi = detail::Query(process, address);
    void* const region_next =
      static_cast<std::uint8_t*>(mbi.BaseAddress) + mbi.RegionSize;

    void* const address_end = static_cast<std::uint8_t*>(address) + len;
    if (address_end <= region_next)
    {
      WriteUnchecked(process, address, data, len);

      protect_guard.Restore();

      return;
    }
    else
    {
      std::size_t const len_new =
        reinterpret_cast<std::uintptr_t>(region_next) -
        reinterpret_cast<std::uintptr_t>(address);

      WriteUnchecked(process, address, data, len_new);

      protect_guard.Restore();

      address = static_cast<std::uint8_t*>(address) + len_new;
      data = static_cast<std::uint8_t const*>(data) + len_new;
      len -= len_new;
    }
  }
}

template <typename T>
void WriteImpl(Process const& process, PVOID address, T const& data)
{
  HADESMEM_DETAIL_STATIC_ASSERT(detail::IsTriviallyCopyable<T>::value);

  HADESMEM_DETAIL_ASSERT(address != nullptr);

  WriteImpl(process, address, std::addressof(data), sizeof(data));
}
}
}

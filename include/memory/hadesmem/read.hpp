// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>
#include <cassert>
#include <cstddef>
#include <exception>
#include <type_traits>

#include <windows.h>

#include "hadesmem/error.hpp"
#include "hadesmem/protect.hpp"
#include "hadesmem/detail/read_impl.hpp"
#include "hadesmem/detail/type_traits.hpp"
#include "hadesmem/detail/query_region.hpp"
#include "hadesmem/detail/protect_guard.hpp"
#include "hadesmem/detail/static_assert.hpp"

// NOTE: Reads which span across region boundaries are not explicitly handled 
// or supported. They may work simply by chance (or if the user changes the 
// memory page protections preemptively in preparation for the read), however 
// this is not guaranteed to work, even in the aforementioned scenario.

namespace hadesmem
{

class Process;

namespace detail
{

template <typename T>
T ReadUnchecked(Process const& process, PVOID address)
{
  HADESMEM_STATIC_ASSERT(detail::IsTriviallyCopyable<T>::value);
  
  T data;
  ReadUnchecked(process, address, std::addressof(data), sizeof(data));
  return data;
}

}

template <typename T>
T Read(Process const& process, PVOID address)
{
  HADESMEM_STATIC_ASSERT(detail::IsTriviallyCopyable<T>::value);
  
  T data;
  detail::Read(process, address, std::addressof(data), sizeof(data));
  return data;
}

template <typename T, std::size_t N>
std::array<T, N> Read(Process const& process, PVOID address)
{
  HADESMEM_STATIC_ASSERT(detail::IsTriviallyCopyable<T>::value);

  HADESMEM_STATIC_ASSERT(detail::IsDefaultConstructible<T>::value);

  std::array<T, N> data;
  detail::Read(process, address, data.data(), sizeof(T) * N);
  return data;
}

// TODO: Clean up this function.
template <typename T>
std::basic_string<T> ReadString(Process const& process, PVOID address, 
  std::size_t chunk_len = 128)
{
  HADESMEM_STATIC_ASSERT(detail::IsCharType<T>::value);

  assert(chunk_len != 0);

  detail::ProtectGuard protect_guard(&process, address, 
    detail::ProtectGuardType::kRead);

  std::basic_string<T> data;

  MEMORY_BASIC_INFORMATION const mbi = detail::Query(process, address);
  PVOID const region_end = static_cast<PBYTE>(mbi.BaseAddress) + 
    mbi.RegionSize;

  T* cur = static_cast<T*>(address);
  while (cur + 1 < region_end)
  {
    std::size_t const len_to_end = reinterpret_cast<DWORD_PTR>(region_end) - 
      reinterpret_cast<DWORD_PTR>(cur);
    std::size_t const buf_len_bytes = (std::min)(chunk_len * sizeof(T), 
      len_to_end);
    std::size_t const buf_len = buf_len_bytes / sizeof(T);

    std::vector<T> buf(buf_len);
    detail::ReadUnchecked(process, cur, buf.data(), buf.size() * sizeof(T));

    auto const iter = std::find(std::begin(buf), std::end(buf), T());
    std::copy(std::begin(buf), iter, std::back_inserter(data));

    if (iter != std::end(buf))
    {
      protect_guard.Restore();
      return data;
    }

    cur += buf_len;
  }

  BOOST_THROW_EXCEPTION(Error() << 
    ErrorString("Attempt to read across a region boundary."));
}

template <typename T>
std::vector<T> ReadVector(Process const& process, PVOID address, 
  std::size_t size)
{
  HADESMEM_STATIC_ASSERT(detail::IsTriviallyCopyable<T>::value);

  HADESMEM_STATIC_ASSERT(detail::IsDefaultConstructible<T>::value);
  
  std::vector<T> data(size);
  detail::Read(process, address, data.data(), sizeof(T) * size);
  return data;
}

}

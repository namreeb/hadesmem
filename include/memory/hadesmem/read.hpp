// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>
#include <cstddef>
#include <exception>
#include <type_traits>

#include <windows.h>

#include <hadesmem/error.hpp>
#include <hadesmem/protect.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/read_impl.hpp>
#include <hadesmem/detail/type_traits.hpp>
#include <hadesmem/detail/query_region.hpp>
#include <hadesmem/detail/protect_guard.hpp>
#include <hadesmem/detail/static_assert.hpp>

// TODO: Support custom string, vector, etc types. Also support custom 
// allocators, traits, etc.

namespace hadesmem
{
  
template <typename T>
inline T Read(Process const& process, PVOID address)
{
  HADESMEM_DETAIL_ASSERT(address != nullptr);
  
  return detail::ReadImpl<T>(process, address);
}

template <typename T, std::size_t N>
inline std::array<T, N> Read(Process const& process, PVOID address)
{
  HADESMEM_DETAIL_STATIC_ASSERT(N != 0);

  HADESMEM_DETAIL_ASSERT(address != nullptr);

  return detail::ReadImpl<std::array<T, N>>(process, address);
}

template <typename T, std::size_t N, typename OutputIterator>
inline void Read(Process const& process, PVOID address, OutputIterator out)
{
  // TODO: Iterator checks for type and category.

  HADESMEM_DETAIL_STATIC_ASSERT(N != 0);

  HADESMEM_DETAIL_ASSERT(address != nullptr);

  auto const data = detail::ReadImpl<std::array<T, N>>(process, address);
  std::copy(&data[0], &data[0] + N, out);
}

template <typename T>
inline std::vector<T> ReadVector(
  Process const& process, 
  PVOID address, 
  std::size_t count);

template <typename T, typename OutputIterator>
inline void Read(
  Process const& process, 
  PVOID address, 
  std::size_t n, 
  OutputIterator out)
{
  HADESMEM_DETAIL_ASSERT(address != nullptr);
  HADESMEM_DETAIL_ASSERT(n != 0);

  auto const data = ReadVector<T>(process, address, n);
  std::copy(std::begin(data), std::end(data), out);
}

// TODO: Clean up this function.
template <typename T, typename OutputIterator>
void ReadStringEx(
  Process const& process, 
  PVOID address, 
  OutputIterator data, 
  std::size_t chunk_len)
{
  HADESMEM_DETAIL_STATIC_ASSERT(detail::IsCharType<T>::value);

  HADESMEM_DETAIL_ASSERT(chunk_len != 0);
  
  // TODO: Iterator checks for type and category.

  for (;;)
  {
    detail::ProtectGuard protect_guard(process, address, 
      detail::ProtectGuardType::kRead);

    MEMORY_BASIC_INFORMATION const mbi = detail::Query(process, address);
    PVOID const region_next = static_cast<PBYTE>(mbi.BaseAddress) + 
      mbi.RegionSize;

    T* cur = static_cast<T*>(address);
    while (cur + 1 <= region_next)
    {
      std::size_t const len_to_end = 
        reinterpret_cast<DWORD_PTR>(region_next) - 
        reinterpret_cast<DWORD_PTR>(cur);
      std::size_t const buf_len_bytes = (std::min)(chunk_len * sizeof(T), 
        len_to_end);
      std::size_t const buf_len = buf_len_bytes / sizeof(T);

      std::vector<T> buf(buf_len);
      detail::ReadUnchecked(process, cur, buf.data(), buf.size() * sizeof(T));

      auto const iter = std::find(std::begin(buf), std::end(buf), T());
      std::copy(std::begin(buf), iter, data);

      if (iter != std::end(buf))
      {
        protect_guard.Restore();
        return;
      }

      cur += buf_len;
    }

    address = region_next;

    protect_guard.Restore();
  }
}

template <typename T, typename OutputIterator>
void ReadString(
  Process const& process, 
  PVOID address, 
  OutputIterator data)
{
  std::size_t const chunk_len = 128;
  return ReadStringEx<T>(process, address, data, chunk_len);
}

// TODO: Support containers with custom traits, allocators, etc.
template <typename T>
std::basic_string<T> ReadStringEx(
  Process const& process, 
  PVOID address, 
  std::size_t chunk_len)
{
  std::basic_string<T> data;
  ReadStringEx<T>(process, address, std::back_inserter(data), chunk_len);
  return data;
}

// TODO: Support containers with custom traits, allocators, etc.
template <typename T>
std::basic_string<T> ReadString(Process const& process, PVOID address)
{
  std::size_t const chunk_len = 128;
  return ReadStringEx<T>(process, address, chunk_len);
}

// TODO: Support containers with custom traits, allocators, etc.
template <typename T>
inline std::vector<T> ReadVector(
  Process const& process, 
  PVOID address, 
  std::size_t count)
{
  HADESMEM_DETAIL_STATIC_ASSERT(detail::IsTriviallyCopyable<T>::value);
  HADESMEM_DETAIL_STATIC_ASSERT(detail::IsDefaultConstructible<T>::value);
  
  HADESMEM_DETAIL_ASSERT(address != nullptr);
  HADESMEM_DETAIL_ASSERT(count != 0);
  
  std::vector<T> data(count);
  detail::ReadImpl(process, address, data.data(), sizeof(T) * count);
  return data;
}

template <typename T, typename OutputIterator>
inline void ReadVector(
  Process const& process, 
  PVOID address, 
  std::size_t count, 
  OutputIterator out)
{
  // TODO: Iterator checks for type and category.

  HADESMEM_DETAIL_STATIC_ASSERT(detail::IsTriviallyCopyable<T>::value);
  HADESMEM_DETAIL_STATIC_ASSERT(detail::IsDefaultConstructible<T>::value);
  
  HADESMEM_DETAIL_ASSERT(address != nullptr);
  HADESMEM_DETAIL_ASSERT(count != 0);

  std::vector<T> data = ReadVector<T>(process, address, count);
  std::copy(std::begin(data), std::end(data), out);
}

}

// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <array>
#include <string>
#include <vector>
#include <cstddef>
#include <exception>
#include <type_traits>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/assert.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include <windows.h>

#include "hadesmem/error.hpp"
#include "hadesmem/protect.hpp"
#include "hadesmem/detail/read_impl.hpp"
#include "hadesmem/detail/type_traits.hpp"
#include "hadesmem/detail/query_region.hpp"
#include "hadesmem/detail/protect_guard.hpp"
#include "hadesmem/detail/static_assert.hpp"

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
  ReadUnchecked(process, address, &data, sizeof(data));
  return data;
}

}

template <typename T>
T Read(Process const& process, PVOID address)
{
  HADESMEM_STATIC_ASSERT(detail::IsTriviallyCopyable<T>::value);
  
  T data;
  detail::Read(process, address, &data, sizeof(data));
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

template <typename T>
std::basic_string<T> ReadString(Process const& process, PVOID address, 
  std::size_t chunk_len = 128)
{
  HADESMEM_STATIC_ASSERT(detail::IsCharType<T>::value);

  BOOST_ASSERT(chunk_len != 0);

  detail::ProtectGuard protect_guard(&process, address, 
    detail::ProtectGuardType::kRead);

  std::basic_string<T> data;

  PBYTE cur_address = static_cast<PBYTE>(address);
  MEMORY_BASIC_INFORMATION const mbi = detail::Query(process, address);
  PBYTE const region_end = static_cast<PBYTE>(mbi.BaseAddress) + 
    mbi.RegionSize;

  for (;;)
  {
    std::size_t cur_chunk_len = 0;
    std::size_t cur_chunk_size = 0;

    if (cur_address + (chunk_len * sizeof(T)) > region_end)
    {
      cur_chunk_size = reinterpret_cast<std::size_t>(region_end - 
        reinterpret_cast<DWORD_PTR>(cur_address));
      cur_chunk_len = (cur_chunk_size - (cur_chunk_size % sizeof(T))) / 
        sizeof(T);
    }
    else
    {
      cur_chunk_size = chunk_len * sizeof(T);
      cur_chunk_len = chunk_len;
    }

    std::vector<BYTE> buf(cur_chunk_size);
    detail::Read(process, cur_address, buf.data(), cur_chunk_size);

    T const* buf_beg = static_cast<T const*>(static_cast<void const*>(
      buf.data()));
    T const* buf_end = buf_beg + cur_chunk_len;
    for (T current = *buf_beg; current != T() && buf_beg != buf_end; 
      current = *++buf_beg)
    {
      data.push_back(current);
    }

    if (buf_beg != buf_end || cur_address + cur_chunk_size == region_end)
    {
      break;
    }

    cur_address += cur_chunk_size;
  }

  protect_guard.Restore();

  return data;
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

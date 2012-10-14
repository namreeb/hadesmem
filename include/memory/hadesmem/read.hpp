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

#include <type_traits>

#include <windows.h>

#include "hadesmem/detail/read_impl.hpp"
#include "hadesmem/detail/type_traits.hpp"
#include "hadesmem/detail/static_assert.hpp"

namespace hadesmem
{

class Process;

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
std::basic_string<T> ReadString(Process const& process, PVOID address)
{
  HADESMEM_STATIC_ASSERT(detail::IsCharType<T>::value);

  std::basic_string<T> data;
  
  // TODO: Optimize to only check page protections once, also look into 
  // reading data in chunks rather than byte-by-byte.
  T* address_real = static_cast<T*>(address);
  for (T current = Read<T>(process, address_real); 
    current != T(); 
    ++address_real, current = Read<T>(process, address_real))
  {
    data.push_back(current);
  }
  
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

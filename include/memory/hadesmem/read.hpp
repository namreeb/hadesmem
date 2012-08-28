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

namespace hadesmem
{

class Process;

template <typename T>
T Read(Process const& process, PVOID address)
{
  // TODO: Update to use std::is_trivially_copyable trait when available in 
  // GCC.
  static_assert(std::is_pod<T>::value, "Read: T must be trivially copyable.");
  
  T data;
  detail::Read(process, address, &data, sizeof(data));
  return data;
}

template <typename T, std::size_t N>
std::array<T, N> Read(Process const& process, PVOID address)
{
  // TODO: Update to use std::is_trivially_copyable trait when available in 
  // GCC.
  static_assert(std::is_pod<T>::value, "Read: T must be trivially copyable.");

  std::array<T, N> data;
  detail::Read(process, address, data.data(), sizeof(T) * N);
  return data;
}

// TODO: Support other string types that model the same STL container 
// style (e.g. boost::basic_string). Perhaps remove the enable_if altogether 
// and just rely on users to use the API with sensible types (and improve 
// diagnostics for cases where dependent type names or functions don't 
// exist through use of BCCL or similar).
template <typename T>
T ReadString(Process const& process, PVOID address, 
  typename std::enable_if<
    std::is_same<
      T, 
      std::basic_string<
        typename T::value_type, 
        typename T::traits_type, 
        typename T::allocator_type
        >
      >::value, 
    T
    >::type* /*dummy*/ = nullptr)
{
  typedef typename T::value_type CharT;

  // TODO: Update to use std::is_trivially_copyable trait when available in 
  // GCC.
  static_assert(std::is_pod<CharT>::value, "ReadString: Character type of "
    "string must be trivially copyable.");
  
  T data;
  
  // TODO: Optimize to only check page protections once, also look into 
  // reading data in chunks rather than byte-by-byte.
  CharT* address_real = static_cast<CharT*>(address);
  for (CharT current = Read<CharT>(process, address_real); 
    current != CharT(); 
    ++address_real, current = Read<CharT>(process, address_real))
  {
    data.push_back(current);
  }
  
  return data;
}

// TODO: Support other container types that model the same STL container 
// style (e.g. boost::vector). Perhaps remove the enable_if altogether 
// and just rely on users to use the API with sensible types (and improve 
// diagnostics for cases where dependent type names or functions don't 
// exist through use of BCCL or similar).
template <typename T>
T ReadVector(Process const& process, PVOID address, std::size_t size, 
  typename std::enable_if<
    std::is_same<
      T, 
      std::vector<
        typename T::value_type, 
        typename T::allocator_type
        >
      >::value, 
    T
    >::type* /*dummy*/ = nullptr)
{
  typedef typename T::value_type ValueT;

  // TODO: Update to use std::is_trivially_copyable trait when available in 
  // GCC.
  static_assert(std::is_pod<ValueT>::value, "ReadVector: Value type of vector "
    "must be trivially copyable.");
  
  T data(size);
  detail::Read(process, address, data.data(), sizeof(ValueT) * size);
  return data;
}

}

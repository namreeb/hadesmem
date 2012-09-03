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

namespace hadesmem
{

class Process;

template <typename T>
T Read(Process const& process, PVOID address)
{
  // TODO: Update to use std::is_trivially_copyable trait when available in 
  // libstdc++.
  static_assert(std::is_trivial<T>::value, "Read: T must be trivially "
    "copyable.");
  
  T data;
  detail::Read(process, address, &data, sizeof(data));
  return data;
}

template <typename T, std::size_t N>
std::array<T, N> Read(Process const& process, PVOID address)
{
  // TODO: Update to use std::is_trivially_copyable trait when available in 
  // libstdc++.
  static_assert(std::is_trivial<T>::value, "Read: T must be trivially "
    "copyable.");

  static_assert(std::is_default_constructible<T>::value, "Read: T must be "
    "default constructible.");

  std::array<T, N> data;
  detail::Read(process, address, data.data(), sizeof(T) * N);
  return data;
}

template <typename T>
std::basic_string<T> ReadString(Process const& process, PVOID address)
{
  static_assert(detail::IsCharType<T>::value, "WriteString: Invalid "
    "character type.");

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
  // TODO: Update to use std::is_trivially_copyable trait when available in 
  // libstdc++.
  static_assert(std::is_trivial<T>::value, "ReadVector: Value type of "
    "vector must be trivially copyable.");

  static_assert(std::is_default_constructible<T>::value, "ReadVector: Value "
    "type of vector must be default constructible.");
  
  std::vector<T> data(size);
  detail::Read(process, address, data.data(), sizeof(T) * size);
  return data;
}

}

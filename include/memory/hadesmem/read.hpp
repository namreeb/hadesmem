// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

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
  static_assert(std::is_pod<T>::value, "Read: T must be POD.");
  
  T data;
  detail::Read(process, address, &data, sizeof(data));
  return data;
}

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
  
  static_assert(std::is_pod<CharT>::value, "ReadString: Character type of "
    "string must be POD.");
  
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
  
  static_assert(std::is_pod<ValueT>::value, "ReadVector: Value type of vector "
    "must be POD.");
  
  T data(size);
  detail::Read(process, address, data.data(), sizeof(ValueT) * size);
  return data;
}

}

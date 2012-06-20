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

#include "hadesmem/detail/write_impl.hpp"

namespace hadesmem
{

class Process;

template <typename T>
void Write(Process const& process, PVOID address, T const& data)
{
  static_assert(std::is_pod<T>::value, "Write: T must be POD.");
  
  detail::Write(process, address, &data, sizeof(data));
}

template <typename T>
void WriteString(Process const& process, PVOID address, T const& data, 
  typename std::enable_if<std::is_same<T, std::basic_string<typename T::value_type, 
  typename T::traits_type, typename T::allocator_type>>::value, T>::type* 
  /*dummy*/ = nullptr)
{
  typedef typename T::value_type CharT;
  
  static_assert(std::is_pod<CharT>::value, "WriteString: Character "
    "type of string must be POD.");
  
  std::size_t const raw_size = (data.size() * sizeof(CharT)) + 1;
  detail::Write(process, address, data.data(), raw_size);
}

template <typename T>
void WriteList(Process const& process, PVOID address, T const& data, 
  typename std::enable_if<std::is_same<T, std::vector<typename T::value_type, 
  typename T::allocator_type>>::value, T>::type* /*dummy*/ = nullptr)
{
  typedef typename T::value_type ValueT;
  
  static_assert(std::is_pod<ValueT>::value, "WriteList: Value type of "
    "vector must be POD.");
  
  std::size_t const raw_size = data.size() * sizeof(ValueT);
  detail::Write(process, address, data.data(), raw_size);
}

}

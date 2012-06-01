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

namespace hadesmem
{

class Process;

namespace detail
{

void Write(Process const& process, PVOID address, LPCVOID in, std::size_t in_size);

}

template <typename T>
void Write(Process const& process, PVOID address, T const& data)
{
  static_assert(std::is_pod<T>::value, "Write: T must be POD.");
  
  detail::Write(process, address, &data, sizeof(data));
}

template <typename T>
void WriteString(Process const& process, PVOID address, T const& data)
{
  typedef typename T::value_type CharT;
  typedef typename T::traits_type TraitsT;
  typedef typename T::allocator_type AllocT;
  
  static_assert(std::is_same<T, std::basic_string<CharT, TraitsT, 
    AllocT>>::value, "WriteString: T must be of type std::basic_string.");
  
  static_assert(std::is_pod<CharT>::value, "WriteString: Character type of "
    "string must be POD.");
  
  std::size_t const raw_size = (data.size() * sizeof(CharT)) + 1;
  detail::Write(process, address, data.data(), raw_size);
}

template <typename T>
void WriteList(Process const& process, PVOID address, T const& data)
{
  typedef typename T::value_type ValueT;
  typedef typename T::allocator_type AllocT;
  
  static_assert(std::is_same<T, std::vector<ValueT, AllocT>>::value, 
    "WriteList: T must be of type std::vector.");
  
  static_assert(std::is_pod<ValueT>::value, "WriteList: Value type of vector "
    "must be POD.");
  
  std::size_t const raw_size = data.size() * sizeof(ValueT);
  detail::Write(process, address, data.data(), raw_size);
}

}

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
  // TODO: Update to use std::is_trivially_copyable trait when available in 
  // GCC.
  static_assert(std::is_pod<T>::value, "Write: T must be trivially copyable.");
  
  detail::Write(process, address, &data, sizeof(data));
}

template <typename T>
void Write(Process const& process, PVOID address, T const* ptr, 
  std::size_t count)
{
  // TODO: Update to use std::is_trivially_copyable trait when available in 
  // GCC.
  static_assert(std::is_pod<T>::value, "Write: T must be trivially copyable.");

  std::size_t const raw_size = std::distance(ptr, ptr + count) * sizeof(T);
  detail::Write(process, address, ptr, raw_size);
}

template <typename T>
void Write(Process const& process, PVOID address, T const* beg, 
  T const* end)
{
  // TODO: Update to use std::is_trivially_copyable trait when available in 
  // GCC.
  static_assert(std::is_pod<T>::value, "Write: T must be trivially copyable.");

  Write(process, address, beg, std::distance(beg, end));
}

// NOTE: This will not write a null terminator.
template <typename T>
void WriteString(Process const& process, PVOID address, T const* const beg, 
  T const* const end)
{
  // TODO: Update to use std::is_trivially_copyable trait when available in 
  // GCC.
  static_assert(std::is_pod<T>::value, "WriteString: Character type of "
    "string must be trivially copyable.");

  Write(process, address, beg, std::distance(beg, end));
}

template <typename T>
void WriteString(Process const& process, PVOID address, 
  std::basic_string<T> const& data)
{
  return WriteString(process, address, data.c_str(), data.c_str() + 
    data.size() + 1);
}

template <typename T>
void WriteString(Process const& process, PVOID address, T const* const str)
{
  // TODO: Update to use std::is_trivially_copyable trait when available in 
  // GCC.
  static_assert(std::is_pod<T>::value, "WriteString: Character type of "
    "string must be trivially copyable.");

  WriteString(process, address, std::basic_string<T>(str));
}

// TODO: Support other container types that model the same STL container 
// style (e.g. boost::vector).
template <typename T>
void WriteVector(Process const& process, PVOID address, 
  std::vector<T> const& data)
{
  // TODO: Update to use std::is_trivially_copyable trait when available in 
  // GCC.
  static_assert(std::is_pod<T>::value, "WriteList: Value type of vector "
    "must be trivially copyable.");
  
  std::size_t const raw_size = data.size() * sizeof(T);
  detail::Write(process, address, data.data(), raw_size);
}

}

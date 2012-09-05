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
#include "hadesmem/detail/type_traits.hpp"

namespace hadesmem
{

class Process;

template <typename T>
void Write(Process const& process, PVOID address, T const& data)
{
  static_assert(detail::IsTriviallyCopyable<T>::value, "T must be trivially "
    "copyable.");
  
  detail::Write(process, address, &data, sizeof(data));
}

template <typename T>
void Write(Process const& process, PVOID address, T const* ptr, 
  std::size_t count)
{
  static_assert(detail::IsTriviallyCopyable<T>::value, "T must be trivially "
    "copyable.");

  std::size_t const raw_size = std::distance(ptr, ptr + count) * sizeof(T);
  detail::Write(process, address, ptr, raw_size);
}

template <typename T>
void Write(Process const& process, PVOID address, T const* beg, 
  T const* end)
{
  static_assert(detail::IsTriviallyCopyable<T>::value, "T must be trivially "
    "copyable.");

  Write(process, address, beg, std::distance(beg, end));
}

// NOTE: This will not write a null terminator.
template <typename T>
void WriteString(Process const& process, PVOID address, T const* const beg, 
  T const* const end)
{
  static_assert(detail::IsCharType<T>::value, "Invalid character type.");

  Write(process, address, beg, std::distance(beg, end));
}

// TODO: Support other container types that model the same STL container 
// style (e.g. boost::basic_string).
template <typename T>
void WriteString(Process const& process, PVOID address, 
  std::basic_string<T> const& data)
{
  static_assert(detail::IsCharType<T>::value, "Invalid character type.");

  return WriteString(process, address, data.c_str(), data.c_str() + 
    data.size() + 1);
}

template <typename T>
void WriteString(Process const& process, PVOID address, T const* const str)
{
  static_assert(detail::IsCharType<T>::value, "Invalid character type.");

  WriteString(process, address, std::basic_string<T>(str));
}

// TODO: Support other container types that model the same STL container 
// style (e.g. boost::vector).
template <typename T>
void WriteVector(Process const& process, PVOID address, 
  std::vector<T> const& data)
{
  static_assert(detail::IsTriviallyCopyable<T>::value, "Value type of vector "
    "must be trivially copyable.");
  
  std::size_t const raw_size = data.size() * sizeof(T);
  detail::Write(process, address, data.data(), raw_size);
}

}

// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <cstddef>

#include <type_traits>

#include <windows.h>

#include <hadesmem/detail/write_impl.hpp>
#include <hadesmem/detail/type_traits.hpp>
#include <hadesmem/detail/static_assert.hpp>

namespace hadesmem
{

// TODO: Write overloads which take iterators (and don't assume that a 
// contiguous block of memory has been passed in).

template <typename T>
inline void Write(Process const& process, PVOID address, T const& data)
{
  HADESMEM_STATIC_ASSERT(detail::IsTriviallyCopyable<T>::value);
  
  HADESMEM_ASSERT(address != nullptr);
  
  detail::WriteImpl(process, address, data);
}

template <typename T>
inline void Write(Process const& process, PVOID address, T const* ptr, 
  std::size_t count)
{
  HADESMEM_STATIC_ASSERT(detail::IsTriviallyCopyable<T>::value);

  HADESMEM_ASSERT(address != nullptr);
  HADESMEM_ASSERT(ptr != nullptr);
  HADESMEM_ASSERT(count != 0);
  
  std::size_t const raw_size = static_cast<std::size_t>(
    std::distance(ptr, ptr + count)) * sizeof(T);
  detail::WriteImpl(process, address, ptr, raw_size);
}

template <typename T>
inline void Write(Process const& process, PVOID address, T const* beg, 
  T const* end)
{
  HADESMEM_STATIC_ASSERT(detail::IsTriviallyCopyable<T>::value);
  
  HADESMEM_ASSERT(address != nullptr);
  HADESMEM_ASSERT(beg != nullptr);
  HADESMEM_ASSERT(end != nullptr);
  
  std::size_t const count = static_cast<std::size_t>(
    std::distance(beg, end));
  Write(process, address, beg, count);
}

// NOTE: This will not write a null terminator.
template <typename T>
inline void WriteString(Process const& process, PVOID address, T const* const beg, 
  T const* const end)
{
  HADESMEM_STATIC_ASSERT(detail::IsCharType<T>::value);
  
  HADESMEM_ASSERT(address != nullptr);
  HADESMEM_ASSERT(beg != nullptr);
  HADESMEM_ASSERT(end != nullptr);
  
  std::size_t const count = static_cast<std::size_t>(
    std::distance(beg, end));
  Write(process, address, beg, count);
}

template <typename T>
inline void WriteString(Process const& process, PVOID address, 
  std::basic_string<T> const& data)
{
  HADESMEM_STATIC_ASSERT(detail::IsCharType<T>::value);
  
  HADESMEM_ASSERT(address != nullptr);

  return WriteString(process, address, data.c_str(), data.c_str() + 
    data.size() + 1);
}

template <typename T>
inline void WriteString(Process const& process, PVOID address, 
  T const* const str)
{
  HADESMEM_STATIC_ASSERT(detail::IsCharType<T>::value);
  
  HADESMEM_ASSERT(address != nullptr);
  HADESMEM_ASSERT(str != nullptr);

  WriteString(process, address, std::basic_string<T>(str));
}

template <typename T>
inline void WriteVector(Process const& process, PVOID address, 
  std::vector<T> const& data)
{
  HADESMEM_STATIC_ASSERT(detail::IsTriviallyCopyable<T>::value);
  
  HADESMEM_ASSERT(address != nullptr);
  HADESMEM_ASSERT(!data.empty());

  std::size_t const raw_size = data.size() * sizeof(T);
  detail::WriteImpl(process, address, data.data(), raw_size);
}

}

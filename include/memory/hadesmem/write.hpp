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

// NOTE: Writes which span across region boundaries are not explicitly handled 
// or supported. They may work simply by chance (or if the user changes the 
// memory page protections preemptively in preparation for the read), however 
// this is not guaranteed to work, even in the aforementioned scenario.

namespace hadesmem
{

class Process;

template <typename T>
void Write(Process const& process, PVOID address, T const& data)
{
  HADESMEM_STATIC_ASSERT(detail::IsTriviallyCopyable<T>::value);
  
  detail::Write(process, address, std::addressof(data), sizeof(data));
}

template <typename T>
void Write(Process const& process, PVOID address, T const* ptr, 
  std::size_t count)
{
  HADESMEM_STATIC_ASSERT(detail::IsTriviallyCopyable<T>::value);

  std::size_t const raw_size = static_cast<std::size_t>(
    std::distance(ptr, ptr + count)) * sizeof(T);
  detail::Write(process, address, ptr, raw_size);
}

template <typename T>
void Write(Process const& process, PVOID address, T const* beg, 
  T const* end)
{
  HADESMEM_STATIC_ASSERT(detail::IsTriviallyCopyable<T>::value);

  std::size_t const count = static_cast<std::size_t>(
    std::distance(beg, end));
  Write(process, address, beg, count);
}

// NOTE: This will not write a null terminator.
template <typename T>
void WriteString(Process const& process, PVOID address, T const* const beg, 
  T const* const end)
{
  HADESMEM_STATIC_ASSERT(detail::IsCharType<T>::value);

  std::size_t const count = static_cast<std::size_t>(
    std::distance(beg, end));
  Write(process, address, beg, count);
}

template <typename T>
void WriteString(Process const& process, PVOID address, 
  std::basic_string<T> const& data)
{
  HADESMEM_STATIC_ASSERT(detail::IsCharType<T>::value);

  return WriteString(process, address, data.c_str(), data.c_str() + 
    data.size() + 1);
}

template <typename T>
void WriteString(Process const& process, PVOID address, T const* const str)
{
  HADESMEM_STATIC_ASSERT(detail::IsCharType<T>::value);

  WriteString(process, address, std::basic_string<T>(str));
}

template <typename T>
void WriteVector(Process const& process, PVOID address, 
  std::vector<T> const& data)
{
  HADESMEM_STATIC_ASSERT(detail::IsTriviallyCopyable<T>::value);
  
  std::size_t const raw_size = data.size() * sizeof(T);
  detail::Write(process, address, data.data(), raw_size);
}

}

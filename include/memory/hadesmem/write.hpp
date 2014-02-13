// Copyright (C) 2010-2014 Joshua Boyce.
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

template <typename T>
inline void Write(Process const& process, PVOID address, T const& data)
{
  HADESMEM_DETAIL_STATIC_ASSERT(detail::IsTriviallyCopyable<T>::value);

  HADESMEM_DETAIL_ASSERT(address != nullptr);

  detail::WriteImpl(process, address, data);
}

template <typename T>
inline void
  Write(Process const& process, PVOID address, T const* ptr, std::size_t count)
{
  HADESMEM_DETAIL_STATIC_ASSERT(detail::IsTriviallyCopyable<T>::value);

  HADESMEM_DETAIL_ASSERT(address != nullptr);
  HADESMEM_DETAIL_ASSERT(ptr != nullptr);
  HADESMEM_DETAIL_ASSERT(count != 0);

  std::size_t const raw_size =
    static_cast<std::size_t>(std::distance(ptr, ptr + count)) * sizeof(T);
  detail::WriteImpl(process, address, ptr, raw_size);
}

template <typename T>
inline void
  Write(Process const& process, PVOID address, T const* beg, T const* end)
{
  HADESMEM_DETAIL_STATIC_ASSERT(detail::IsTriviallyCopyable<T>::value);

  HADESMEM_DETAIL_ASSERT(address != nullptr);
  HADESMEM_DETAIL_ASSERT(beg != nullptr);
  HADESMEM_DETAIL_ASSERT(end != nullptr);

  std::size_t const count = static_cast<std::size_t>(std::distance(beg, end));
  Write(process, address, beg, count);
}

template <typename T,
          typename Traits = std::char_traits<T>,
          typename Alloc = std::allocator<T>>
inline void WriteString(Process const& process,
                        PVOID address,
                        std::basic_string<T, Traits, Alloc> const& data)
{
  HADESMEM_DETAIL_STATIC_ASSERT(detail::IsCharType<T>::value);

  HADESMEM_DETAIL_ASSERT(address != nullptr);

  return Write(process, address, data.c_str(), data.size() + 1);
}

template <typename T>
inline void
  WriteString(Process const& process, PVOID address, T const* const str)
{
  HADESMEM_DETAIL_STATIC_ASSERT(detail::IsCharType<T>::value);

  HADESMEM_DETAIL_ASSERT(address != nullptr);
  HADESMEM_DETAIL_ASSERT(str != nullptr);

  WriteString(process, address, std::basic_string<T>(str));
}

template <typename T, typename Alloc = std::allocator<T>>
inline void WriteVector(Process const& process,
                        PVOID address,
                        std::vector<T, Alloc> const& data)
{
  HADESMEM_DETAIL_STATIC_ASSERT(detail::IsTriviallyCopyable<T>::value);

  HADESMEM_DETAIL_ASSERT(address != nullptr);
  HADESMEM_DETAIL_ASSERT(!data.empty());

  std::size_t const raw_size = data.size() * sizeof(T);
  detail::WriteImpl(process, address, data.data(), raw_size);
}
}

// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstring>
#include <type_traits>

#include <hadesmem/detail/static_assert.hpp>
#include <hadesmem/detail/type_traits.hpp>

namespace hadesmem
{
namespace detail
{
// WARNING: Here be dragons. Use sparingly.
template <typename T,
          typename U,
          int = FuncCallConv<T>::value,
          int = FuncCallConv<U>::value>
inline T AliasCast(U const& u)
{
  // Technically the use of std::is_pod could be relaxed, but this is true for
  // all our use cases so it's good enough.
  HADESMEM_DETAIL_STATIC_ASSERT(std::is_pod<T>::value);
  HADESMEM_DETAIL_STATIC_ASSERT(std::is_pod<U>::value);

  // Technically this could be relaxed, but this is true for all our
  // use cases so it's good enough.
  HADESMEM_DETAIL_STATIC_ASSERT(sizeof(T) == sizeof(U));

  return AliasCastUnchecked<T>(u);
}

// WARNING: Here be dragons. Even more dangerous than its checked equivalent.
// Use only when you have no other choice.
template <typename T,
          typename U,
          int = FuncCallConv<T>::value,
          int = FuncCallConv<U>::value>
inline T AliasCastUnchecked(U const& u)
{
  // Use an intermediate buffer to avoid a strict aliasing violation.
  unsigned char buffer[sizeof(T)];
  std::memcpy(buffer, &u, sizeof(buffer));

  T t;
  std::memcpy(&t, buffer, sizeof(T));
  return t;
}
}
}

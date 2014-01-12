// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <type_traits>

#include <hadesmem/detail/static_assert.hpp>

namespace hadesmem
{

namespace detail
{

template <typename T,
          typename U,
          int DummyCallConvT = FuncCallConv<T>::value,
          int DummyCallConvU = FuncCallConv<U>::value>
inline T UnionCast(U const& u)
{
  // std::is_pod seems to be broken under Intel C++ 2013 SP1 Update 1.
  // TODO: Fix this properly.
#if !defined(HADESMEM_INTEL)
  // Technically the use of std::is_pod could be relaxed, but this is true for 
  // all our use cases so it's good enough.
  HADESMEM_DETAIL_STATIC_ASSERT(std::is_pod<T>::value);
  HADESMEM_DETAIL_STATIC_ASSERT(std::is_pod<U>::value);
#endif // #if !defined(HADESMEM_INTEL)

  // Technically this could be relaxed, but this is true for all our
  // use cases so it's good enough.
  HADESMEM_DETAIL_STATIC_ASSERT(sizeof(T) == sizeof(U));

  union Conv
  {
    T t;
    U u;
  };
  Conv conv;
  conv.u = u;
  // Technically this is (AFAICT) undefined behaviour, however all
  // the major compilers (and probably the minor ones too) support
  // this as the de-facto method for type-punning.
  return conv.t;
}

// WARNING: Here be dragons. Use with extreme caution. Undefined
// behaviour galore.
template <typename T,
          typename U,
          int DummyCallConvT = FuncCallConv<T>::value,
          int DummyCallConvU = FuncCallConv<U>::value>
inline T UnionCastUnchecked(U const& u)
{
  union Conv
  {
    T t;
    U u;
  };
  Conv conv;
  conv.u = u;
  return conv.t;
}
}
}

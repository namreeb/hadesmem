// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <type_traits>

#include "hadesmem/config.hpp"

namespace hadesmem
{

namespace detail
{

template <typename T>
struct IsCharType
{
  static bool const value = std::is_same<T, char>::value || 
    std::is_same<T, signed char>::value || 
    std::is_same<T, unsigned char>::value || 
    std::is_same<T, wchar_t>::value || 
    std::is_same<T, char16_t>::value || 
    std::is_same<T, char32_t>::value;
};

template <typename T>
struct IsTriviallyCopyable
{
#if defined(HADESMEM_GCC) || defined(HADESMEM_CLANG)
  // TODO: Update to use std::is_trivially_copyable trait when available in 
  // libstdc++.
  static bool const value = std::is_trivial<T>::value;
#else
  static bool const value = std::is_trivially_copyable<T>::value;
#endif
};

template <typename T>
struct IsDefaultConstructible
{
#if defined(HADESMEM_CLANG)
  // TODO: Update to use std::is_default_constructible trait when available in 
  // libstdc++. In the meantime it should be safe to mark every type as 
  // default constructible as even if it isn't, it will result in a 
  // compile-time error anyway.
  static bool const value = true;
#else
  static bool const value = std::is_default_constructible<T>::value;
#endif
};

template <typename T>
struct UnderlyingType
{
#if defined(HADESMEM_CLANG)
  // The Clang toolchain I'm using currently does not support 
  // std::underlying_type. Using 'unsigned int' here as there's nothing else 
  // to do other than pick a sensible default (which I can do because I know 
  // all the places this trait is being used).
  // TODO: Update to use std::underlying_type when available.
  typedef unsigned int type;
#else
  typedef typename std::underlying_type<T>::type type;
#endif
};

}

}

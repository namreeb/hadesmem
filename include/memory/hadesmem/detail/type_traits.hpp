// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <type_traits>

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

}

}

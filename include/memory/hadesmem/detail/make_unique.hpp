// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <memory>

#include <hadesmem/config.hpp>

// Will not work for arrays. The implementation in N3656 is better, but MSVC 
// does not (at the time of writing) have deleted functions, so this will 
// have to do.

namespace hadesmem
{

namespace detail
{

#if !defined(HADESMEM_DETAIL_NO_VARIADIC_TEMPLATES)

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

#else // #if !defined(HADESMEM_DETAIL_NO_VARIADIC_TEMPLATES)

// Not using Boost.Preprocessor for the non-variadic implementation of this 
// because it is a 'detail' API and so users (should) have no need to 
// configure it.

#if defined(HADESMEM_MSVC)
#pragma warning(push)
#pragma warning(disable: 4100)
#endif // #if defined(HADESMEM_MSVC)

template<typename T>
std::unique_ptr<T> make_unique()
{
  return std::unique_ptr<T>(new T());
}

template<typename T, typename A1>
std::unique_ptr<T> make_unique(A1&& a1)
{
  return std::unique_ptr<T>(new T(std::forward<A1>(a1)));
}

template<typename T, typename A1, typename A2>
std::unique_ptr<T> make_unique(A1&& a1, A2&& a2)
{
  return std::unique_ptr<T>(new T(std::forward<A1>(a1), 
    std::forward<A2>(a2)));
}

template<typename T, typename A1, typename A2, typename A3>
std::unique_ptr<T> make_unique(A1&& a1, A2&& a2, A3&& a3)
{
  return std::unique_ptr<T>(new T(std::forward<A1>(a1), 
    std::forward<A2>(a2), std::forward<A3>(a3)));
}

template<typename T, typename A1, typename A2, typename A3, typename A4>
std::unique_ptr<T> make_unique(A1&& a1, A2&& a2, A3&& a3, A4&& a4)
{
  return std::unique_ptr<T>(new T(std::forward<A1>(a1), 
    std::forward<A2>(a2), std::forward<A3>(a3), std::forward<A4>(a4)));
}

template<typename T, typename A1, typename A2, typename A3, typename A4, 
  typename A5>
std::unique_ptr<T> make_unique(A1&& a1, A2&& a2, A3&& a3, A4&& a4, A5&& a5)
{
  return std::unique_ptr<T>(new T(std::forward<A1>(a1), 
    std::forward<A2>(a2), std::forward<A3>(a3), std::forward<A4>(a4), 
    std::forward<A5>(a5)));
}

#if defined(HADESMEM_MSVC)
#pragma warning(pop)
#endif // #if defined(HADESMEM_MSVC)

#endif // #if !defined(HADESMEM_DETAIL_NO_VARIADIC_TEMPLATES)

}

}

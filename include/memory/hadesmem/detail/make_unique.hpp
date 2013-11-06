// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <memory>

#include <hadesmem/config.hpp>

// Will not work for arrays. The implementation in N3656 is better, but MSVC 
// does not (at the time of writing) have deleted functions, so this will 
// have to do.
// TODO: Fix this implementation for Dev12 RTM.

namespace hadesmem
{

namespace detail
{

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

}

}

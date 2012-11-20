// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <type_traits>

#include "hadesmem/config.hpp"
#include "hadesmem/detail/static_assert.hpp"

namespace hadesmem
{

namespace detail
{

template <typename From, typename To>
union UnionCastImpl
{
public:
  // C++11 does not require members of a union to be POD, but this union 
  // should only be used to perform casts between known safe (for a very 
  // lax definition of 'safe') types which are always POD.
  HADESMEM_STATIC_ASSERT(std::is_pod<To>::value);
  HADESMEM_STATIC_ASSERT(std::is_pod<From>::value);

  // C++ does not require members of a union to have the same size or 
  // alignment requirements, however because this particular union is only to 
  // be used to convert between known 'safe' types of the same size, this 
  // should always be true.
  HADESMEM_STATIC_ASSERT(sizeof(From) == sizeof(To));
  HADESMEM_STATIC_ASSERT(std::alignment_of<From>::value == 
    std::alignment_of<To>::value);

  explicit UnionCastImpl(From from) HADESMEM_NOEXCEPT
    : from_(from)
  { }

  To GetTo() const HADESMEM_NOEXCEPT
  {
    return to_;
  }

private:
  From from_;
  To to_;
};

template <typename To, typename From>
To UnionCast(From from) HADESMEM_NOEXCEPT
{
  return UnionCastImpl<From, To>(from).GetTo();
}

}

}

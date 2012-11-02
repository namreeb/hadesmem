// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <type_traits>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/config.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

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

  explicit UnionCastImpl(From from) BOOST_NOEXCEPT
    : from_(from)
  { }

  To GetTo() const BOOST_NOEXCEPT
  {
    return to_;
  }

private:
  From from_;
  To to_;
};

template <typename To, typename From>
To UnionCast(From from) BOOST_NOEXCEPT
{
  return UnionCastImpl<From, To>(from).GetTo();
}

}

}

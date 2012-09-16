// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

namespace hadesmem
{

namespace detail
{

template <typename From, typename To>
union UnionCast
{
public:
  UnionCast(From from)
    : from_(from)
  { }

  To GetTo() const
  {
    return to_;
  }

private:
  From from_;
  To to_;
};

}

}

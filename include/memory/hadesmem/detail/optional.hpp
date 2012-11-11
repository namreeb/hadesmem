// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <type_traits>

#include "hadesmem/error.hpp"
#include "hadesmem/config.hpp"

namespace hadesmem
{

namespace detail
{

template <typename T>
class Optional
{
public:
  Optional() HADESMEM_NOEXCEPT
    : valid_(false), 
    t_()
  { }

  explicit Optional(T const& t)
    : valid_(true), 
    t_()
  {
    Create(t);
  }

  Optional& operator=(T const& t)
  {
    Destroy();
    Create(t);

    return *this;
  }

  ~Optional()
  {
    Destroy();
  }

  bool IsValid() const HADESMEM_NOEXCEPT
  {
    return valid_;
  }

  T& operator*() HADESMEM_NOEXCEPT
  {
    return const_cast<T&>(static_cast<Optional const*>(this)->operator*());
  }

  T const& operator*() const HADESMEM_NOEXCEPT
  {
    if (IsValid())
    {
      return *reinterpret_cast<T const*>(&t_);
    }

    BOOST_THROW_EXCEPTION(Error() << 
      ErrorString("Type is invalid."));
  }

  T* operator->() HADESMEM_NOEXCEPT
  {
    return const_cast<T*>(static_cast<Optional const*>(this)->operator->());
  }

  T const* operator->() const HADESMEM_NOEXCEPT
  {
    if (IsValid())
    {
      return reinterpret_cast<T const*>(&t_);
    }

    BOOST_THROW_EXCEPTION(Error() << 
      ErrorString("Type is invalid."));
  }

private:
  Optional(Optional const& other) HADESMEM_DELETED_FUNCTION;
  Optional& operator=(Optional const& other) HADESMEM_DELETED_FUNCTION;

  void Destroy()
  {
    if (IsValid())
    {
      reinterpret_cast<T*>(&t_)->~T();
      valid_ = false;
    }
  }

  void Create(T const& t)
  {
    new (&t_) T(t);
    valid_ = true;
  }

  bool valid_;
  typename std::aligned_storage<sizeof(T), 
    std::alignment_of<T>::value>::type t_;
};

}

}

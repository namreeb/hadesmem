// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

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
    t_(nullptr), 
    buf_()
  { }

  explicit Optional(T const& t)
    : valid_(true), 
    t_(nullptr), 
    buf_()
  {
    Create(t);
  }

  Optional(Optional const& other)
    : valid_(false), 
    t_(nullptr), 
    buf_()
  {
    if (other.IsValid())
    {
      Create(*other);
    }
  }

  Optional& operator=(Optional const& other)
  {
    Destroy();

    if (other.IsValid())
    {
      Create(*other);
    }

    return *this;
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
      return *t_;
    }

    HADESMEM_THROW_EXCEPTION(Error() << 
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
      return t_;
    }

    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Type is invalid."));
  }

private:
  void Destroy()
  {
    if (IsValid())
    {
      t_->~T();
      valid_ = false;
    }
  }

  void Create(T const& t)
  {
    t_ = new (&buf_) T(t);
    valid_ = true;
  }

  bool valid_;
  T* t_;
  typename std::aligned_storage<sizeof(T), 
    std::alignment_of<T>::value>::type buf_;
};

}

}

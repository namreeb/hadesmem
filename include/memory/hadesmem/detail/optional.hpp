// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <utility>
#include <type_traits>

#include <hadesmem/config.hpp>

// TODO: operator== and operator<.

// TODO: Implement same interface as std::optional<T> to make switching later 
// easier.

namespace hadesmem
{

namespace detail
{

// WARNING: T must have no-throw move, no-throw move assignment and no-throw 
// destruction.
template <typename T>
class Optional
{
private:
  typedef void (Optional::* Boolean)() const;

public:
  Optional() HADESMEM_DETAIL_NOEXCEPT
    : t_(), 
    valid_(false)
  { }

  explicit Optional(T const& t)
    : t_(), 
    valid_(false)
  {
    Construct(t);
  }

  explicit Optional(T&& t)
    : t_(), 
    valid_(false)
  {
    Construct(std::move(t));
  }

  Optional(Optional const& other)
    : t_(), 
    valid_(false)
  {
    Construct(other.Get());
  }

  Optional& operator=(Optional const& other)
  {
    Optional tmp(other);
    *this = std::move(tmp);

    return *this;
  }

  Optional(Optional&& other) HADESMEM_DETAIL_NOEXCEPT
    : t_(), 
    valid_(false)
  {
    Construct(std::move(other.Get()));
    other.valid_ = false;
  }

  Optional& operator=(Optional&& other) HADESMEM_DETAIL_NOEXCEPT
  {
    Destroy();
    Construct(std::move(other->Get()));
    other.valid_ = false;

    return *this;
  }

  Optional& operator=(T const& t)
  {
    Destroy();
    Construct(t);

    return *this;
  }

  Optional& operator=(T&& t)
  {
    Destroy();
    Construct(std::move(t));

    return *this;
  }

  ~Optional()
  {
    Destroy();
  }

  // TODO: Emplacement support. (Including default construction of T.)

#if defined(HADESMEM_DETAIL_NO_EXPLICIT_CONVERSION_OPERATOR)
  operator Boolean() const
  {
    return valid_ ? &Optional::NotComparable : nullptr;
  }
#else // #if defined(HADESMEM_DETAIL_NO_EXPLICIT_CONVERSION_OPERATOR)
  HADESMEM_DETAIL_EXPLICIT_CONVERSION_OPERATOR operator bool() const
  {
    return valid_;
  }
#endif // #if defined(HADESMEM_DETAIL_NO_EXPLICIT_CONVERSION_OPERATOR)

  T& Get()
  {
    return *GetPtr();
  }

  T const& Get() const
  {
    return *GetPtr();
  }

  T& operator*()
  {
    return Get();
  }

  T const& operator*() const
  {
    return Get();
  }

  T* GetPtr()
  {
    return static_cast<T*>(static_cast<void*>(&t_));
  }

  T* GetPtr() const
  {
    return static_cast<T const*>(static_cast<void const*>(&t_));
  }

  T* operator->()
  {
    return GetPtr();
  }

  T const* operator->() const
  {
    return GetPtr();
  }

private:
  void NotComparable() const
  { }

  template <typename U>
  void Construct(U&& u)
  {
    if (valid_)
    {
      Destroy();
    }

    new (&t_) T(std::forward<U>(u));
    valid_ = true;
  }

  void Destroy() HADESMEM_DETAIL_NOEXCEPT
  {
    if (valid_)
    {
      GetPtr()->~T();
      valid_ = false;
    }
  }

  typename std::aligned_storage<sizeof(T), std::alignment_of<T>::value>::type t_;
  bool valid_;
};

}

}

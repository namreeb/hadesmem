// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <type_traits>
#include <utility>

#include <hadesmem/config.hpp>

// TODO: Implement same interface as std::optional<T> to make switching later
// easier.

namespace hadesmem
{

namespace detail
{

// WARNING: T must have no-throw move, no-throw move assignment and
// no-throw destruction.
// TODO: static_assert the preconditions above.
template <typename T> class Optional
{
public:
  HADESMEM_DETAIL_CONSTEXPR Optional() HADESMEM_DETAIL_NOEXCEPT : t_(),
                                                                  valid_(false)
  {
  }

  explicit Optional(T const& t) : t_(), valid_(false)
  {
    Construct(t);
  }

  explicit Optional(T&& t) : t_(), valid_(false)
  {
    Construct(std::move(t));
  }

  Optional(Optional const& other) : t_(), valid_(false)
  {
    Construct(other.Get());
  }

  Optional& operator=(Optional const& other)
  {
    Optional tmp(other);
    *this = std::move(tmp);

    return *this;
  }

  Optional(Optional&& other) HADESMEM_DETAIL_NOEXCEPT : t_(), valid_(false)
  {
    Construct(std::move(other.Get()));
    other.valid_ = false;
  }

  Optional& operator=(Optional&& other) HADESMEM_DETAIL_NOEXCEPT
  {
    Destroy();
    Construct(std::move(other.Get()));
    other.valid_ = false;

    return *this;
  }

  Optional& operator=(T const& t)
  {
    Destroy();
    Construct(t);

    return *this;
  }

  Optional& operator=(T&& t) HADESMEM_DETAIL_NOEXCEPT
  {
    Destroy();
    Construct(std::move(t));

    return *this;
  }

  ~Optional() HADESMEM_DETAIL_NOEXCEPT
  {
    Destroy();
  }

  explicit operator bool() const HADESMEM_DETAIL_NOEXCEPT
  {
    return valid_;
  }

  T& Get() HADESMEM_DETAIL_NOEXCEPT
  {
    return *GetPtr();
  }

  T const& Get() const HADESMEM_DETAIL_NOEXCEPT
  {
    return *GetPtr();
  }

  T& operator*() HADESMEM_DETAIL_NOEXCEPT
  {
    return Get();
  }

  T const& operator*() const HADESMEM_DETAIL_NOEXCEPT
  {
    return Get();
  }

  T* GetPtr() HADESMEM_DETAIL_NOEXCEPT
  {
    return static_cast<T*>(static_cast<void*>(&t_));
  }

  T const* GetPtr() const HADESMEM_DETAIL_NOEXCEPT
  {
    return static_cast<T const*>(static_cast<void const*>(&t_));
  }

  T* operator->() HADESMEM_DETAIL_NOEXCEPT
  {
    return GetPtr();
  }

  T const* operator->() const HADESMEM_DETAIL_NOEXCEPT
  {
    return GetPtr();
  }

private:
  template <typename U> void Construct(U&& u)
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

  std::aligned_storage_t<sizeof(T), std::alignment_of<T>::value> t_;
  bool valid_;
};

template <typename T>
inline bool operator==(Optional<T> const& lhs, Optional<T> const& rhs)
{
  return (!lhs && !rhs) || ((rhs && lhs) && *rhs == *lhs);
}

template <typename T>
inline bool operator!=(Optional<T> const& lhs, Optional<T> const& rhs)
{
  return !(lhs == rhs);
}

template <typename T>
inline bool operator<(Optional<T> const& lhs, Optional<T> const& rhs)
{
  return (!lhs && !rhs) || ((rhs && lhs) && *rhs < *lhs);
}
}
}

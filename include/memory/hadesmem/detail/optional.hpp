// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <type_traits>
#include <utility>

#include <hadesmem/config.hpp>

namespace hadesmem
{
namespace detail
{
template <typename T> class Optional
{
public:
  constexpr Optional() noexcept
  {
  }

  explicit Optional(T const& t)
  {
    Construct(t);
  }

  explicit Optional(T&& t)
  {
    Construct(std::move(t));
  }

  Optional(Optional const& other)
  {
    if (other.valid_)
    {
      Construct(other.Get());
    }
  }

  Optional& operator=(Optional const& other)
  {
    Optional tmp{other};
    *this = std::move(tmp);

    return *this;
  }

  Optional(Optional&& other)
  {
    if (other.valid_)
    {
      Construct(std::move(other.Get()));
    }

    other.valid_ = false;
  }

  Optional& operator=(Optional&& other)
  {
    Destroy();

    if (other.valid_)
    {
      Construct(std::move(other.Get()));
    }

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

  explicit operator bool() const noexcept
  {
    return valid_;
  }

  T& Get() noexcept
  {
    return *GetPtr();
  }

  T const& Get() const noexcept
  {
    return *GetPtr();
  }

  T& operator*() noexcept
  {
    return Get();
  }

  T const& operator*() const noexcept
  {
    return Get();
  }

  T* GetPtr() noexcept
  {
    return static_cast<T*>(static_cast<void*>(&t_));
  }

  T const* GetPtr() const noexcept
  {
    return static_cast<T const*>(static_cast<void const*>(&t_));
  }

  T* operator->() noexcept
  {
    return GetPtr();
  }

  T const* operator->() const noexcept
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

    // Avoid picking up placement new overloads
    ::new (static_cast<void*>(&t_)) T(std::forward<U>(u));
    valid_ = true;
  }

  void Destroy() noexcept
  {
    if (valid_)
    {
      GetPtr()->~T();
      valid_ = false;
    }
  }

  std::aligned_storage_t<sizeof(T), std::alignment_of<T>::value> t_{};
  bool valid_{false};
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

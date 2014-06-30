// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/assert.hpp>

namespace hadesmem
{

namespace detail
{
enum class SRWLockType
{
  Exclusive,
  Shared
};

class AcquireSRWLock
{
public:
  explicit AcquireSRWLock(SRWLOCK* lock, SRWLockType type) HADESMEM_DETAIL_NOEXCEPT
    : lock_{lock}, type_{type}
  {
    HADESMEM_DETAIL_ASSERT(lock_);

    if (type_ == SRWLockType::Exclusive)
    {
      AcquireSRWLockExclusive(lock_);
    }
    else
    {
      AcquireSRWLockShared(lock_);
    }
  }

  AcquireSRWLock(AcquireSRWLock&& other) HADESMEM_DETAIL_NOEXCEPT
    : lock_{other.lock_},
      type_{other.type_}
  {
    other.lock_ = nullptr;
  }

  AcquireSRWLock& operator=(AcquireSRWLock&& other) HADESMEM_DETAIL_NOEXCEPT
  {

    Cleanup();

    lock_ = other.lock_;
    other.lock_ = nullptr;

    type_ = other.type_;

    return *this;
  }

  ~AcquireSRWLock()
  {
    Cleanup();
  }

  void Cleanup() HADESMEM_DETAIL_NOEXCEPT
  {
    if (!lock_)
    {
      return;
    }

    if (type_ == SRWLockType::Exclusive)
    {
      ReleaseSRWLockExclusive(lock_);
    }
    else
    {
      ReleaseSRWLockShared(lock_);
    }

    lock_ = nullptr;
  }

private:
  SRWLOCK* lock_;
  SRWLockType type_;
};
}
}

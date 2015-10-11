// Copyright (C) 2010-2015 Joshua Boyce
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
  _When_(lock_, _Acquires_lock_(lock_)) explicit AcquireSRWLock(
    SRWLOCK* lock, SRWLockType type) noexcept : lock_{lock},
                                                type_{type}
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

  AcquireSRWLock(AcquireSRWLock&& other) noexcept : lock_{other.lock_},
                                                    type_{other.type_}
  {
    other.lock_ = nullptr;
  }

  _When_(lock_, _Requires_lock_held_(lock_)) AcquireSRWLock&
    operator=(AcquireSRWLock&& other) noexcept
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

  _When_(lock_, _Releases_lock_(lock_)) void Cleanup() noexcept
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

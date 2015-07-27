// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>

#include <hadesmem/config.hpp>

namespace hadesmem
{
namespace detail
{
class LastErrorPreserver
{
public:
  LastErrorPreserver() noexcept : last_error_(::GetLastError())
  {
  }

  LastErrorPreserver(LastErrorPreserver const&) = delete;

  LastErrorPreserver& operator=(LastErrorPreserver const&) = delete;

  ~LastErrorPreserver()
  {
    Revert();
  }

  void Update() noexcept
  {
    last_error_ = ::GetLastError();
  }

  void Revert() noexcept
  {
    SetLastError(last_error_);
  }

private:
  DWORD last_error_;
};
}
}

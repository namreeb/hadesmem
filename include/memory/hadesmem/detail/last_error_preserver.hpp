// Copyright (C) 2010-2014 Joshua Boyce.
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
  LastErrorPreserver() HADESMEM_DETAIL_NOEXCEPT : last_error_(::GetLastError())
  {
  }

  LastErrorPreserver(LastErrorPreserver const&) = delete;

  LastErrorPreserver& operator=(LastErrorPreserver const&) = delete;

  ~LastErrorPreserver()
  {
    Revert();
  }

  void Update() HADESMEM_DETAIL_NOEXCEPT
  {
    last_error_ = ::GetLastError();
  }

  void Revert() HADESMEM_DETAIL_NOEXCEPT
  {
    SetLastError(last_error_);
  }

private:
  DWORD last_error_;
};
}
}

// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>

#include <hadesmem/config.hpp>

namespace hadesmem
{

namespace detail
{

class SmartHandle
{
public:
  SmartHandle() HADESMEM_NOEXCEPT;
  
  explicit SmartHandle(HANDLE handle) HADESMEM_NOEXCEPT;

  explicit SmartHandle(HANDLE handle, HANDLE invalid) HADESMEM_NOEXCEPT;

  SmartHandle& operator=(HANDLE handle) HADESMEM_NOEXCEPT;

  SmartHandle(SmartHandle&& other) HADESMEM_NOEXCEPT;

  SmartHandle& operator=(SmartHandle&& other) HADESMEM_NOEXCEPT;

  ~SmartHandle();

  HANDLE GetHandle() const HADESMEM_NOEXCEPT;

  HANDLE GetInvalid() const HADESMEM_NOEXCEPT;

  bool IsValid() const HADESMEM_NOEXCEPT;

  void Cleanup();

private:
  SmartHandle(SmartHandle const& other) HADESMEM_DELETED_FUNCTION;
  SmartHandle& operator=(SmartHandle const& other) HADESMEM_DELETED_FUNCTION;

  void CleanupUnchecked() HADESMEM_NOEXCEPT;

  HANDLE handle_;
  HANDLE invalid_;
};

}

}

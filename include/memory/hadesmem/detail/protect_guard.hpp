// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>

#include "hadesmem/config.hpp"

namespace hadesmem
{

class Process;

namespace detail
{

enum class ProtectGuardType
{
  kRead, 
  kWrite
};

class ProtectGuard
{
public:
  ProtectGuard(Process const* process, PVOID address, ProtectGuardType type);

  ProtectGuard(ProtectGuard&& other) HADESMEM_NOEXCEPT;

  ProtectGuard& operator=(ProtectGuard&& other) HADESMEM_NOEXCEPT;

  ~ProtectGuard();

  void Restore();

  void RestoreUnchecked() HADESMEM_NOEXCEPT;

private:
  ProtectGuard(ProtectGuard const& other) HADESMEM_DELETED_FUNCTION;
  ProtectGuard& operator=(ProtectGuard const& other) HADESMEM_DELETED_FUNCTION;

  Process const* process_;
  PVOID address_;
  ProtectGuardType type_;
  bool can_read_or_write_;
  DWORD old_protect_;
};

}

}

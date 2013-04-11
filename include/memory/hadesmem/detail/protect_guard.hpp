// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <memory>

#include <windows.h>

#include <hadesmem/config.hpp>

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
  explicit ProtectGuard(Process const& process, PVOID address, 
    ProtectGuardType type);

  ProtectGuard(ProtectGuard&& other) HADESMEM_NOEXCEPT;

  ProtectGuard& operator=(ProtectGuard&& other) HADESMEM_NOEXCEPT;

  ~ProtectGuard();

  void Restore();

  void RestoreUnchecked() HADESMEM_NOEXCEPT;

private:
  ProtectGuard(ProtectGuard const& other) HADESMEM_DELETED_FUNCTION;
  ProtectGuard& operator=(ProtectGuard const& other) HADESMEM_DELETED_FUNCTION;

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}

}

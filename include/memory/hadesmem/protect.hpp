// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>

#include <hadesmem/detail/query_region.hpp>
#include <hadesmem/detail/protect_region.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>

namespace hadesmem
{
inline bool CanRead(Process const& process, LPCVOID address)
{
  MEMORY_BASIC_INFORMATION const mbi = detail::Query(process, address);
  return detail::CanRead(mbi);
}

inline bool CanWrite(Process const& process, LPCVOID address)
{
  MEMORY_BASIC_INFORMATION const mbi = detail::Query(process, address);
  return detail::CanWrite(mbi);
}

inline bool CanExecute(Process const& process, LPCVOID address)
{
  MEMORY_BASIC_INFORMATION const mbi = detail::Query(process, address);
  return detail::CanExecute(mbi);
}

inline bool IsGuard(Process const& process, LPCVOID address)
{
  MEMORY_BASIC_INFORMATION const mbi = detail::Query(process, address);
  return detail::IsGuard(mbi);
}

inline bool IsNoCache(Process const& process, LPCVOID address)
{
  MEMORY_BASIC_INFORMATION const mbi = detail::Query(process, address);
  return detail::IsWriteCombine(mbi);
}

inline bool IsWriteCombine(Process const& process, LPCVOID address)
{
  MEMORY_BASIC_INFORMATION const mbi = detail::Query(process, address);
  return detail::IsWriteCombine(mbi);
}

inline bool IsBadProtect(Process const& process, LPCVOID address)
{
  MEMORY_BASIC_INFORMATION const mbi = detail::Query(process, address);
  return detail::IsBadProtect(mbi);
}

inline DWORD Protect(Process const& process, LPVOID address, DWORD protect)
{
  MEMORY_BASIC_INFORMATION const mbi = detail::Query(process, address);
  return detail::Protect(process, mbi, protect);
}
}

// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>

namespace hadesmem
{

namespace detail
{

inline MEMORY_BASIC_INFORMATION Query(Process const& process, LPCVOID address)
{
  MEMORY_BASIC_INFORMATION mbi;
  ::ZeroMemory(&mbi, sizeof(mbi));
  if (::VirtualQueryEx(process.GetHandle(), address, &mbi, sizeof(mbi)) !=
      sizeof(mbi))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                    << ErrorString("VirtualQueryEx failed.")
                                    << ErrorCodeWinLast(last_error));
  }

  return mbi;
}

inline bool CanRead(MEMORY_BASIC_INFORMATION const& mbi)
  HADESMEM_DETAIL_NOEXCEPT
{
  DWORD const read_prot = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY |
                          PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE |
                          PAGE_EXECUTE_WRITECOPY;
  return (mbi.State == MEM_COMMIT) && !!(mbi.Protect & read_prot);
}

inline bool CanWrite(MEMORY_BASIC_INFORMATION const& mbi)
  HADESMEM_DETAIL_NOEXCEPT
{
  DWORD const write_prot = PAGE_READWRITE | PAGE_WRITECOPY |
                           PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
  return (mbi.State == MEM_COMMIT) && !!(mbi.Protect & write_prot);
}

inline bool CanExecute(MEMORY_BASIC_INFORMATION const& mbi)
  HADESMEM_DETAIL_NOEXCEPT
{
  DWORD const exec_prot = PAGE_EXECUTE | PAGE_EXECUTE_READ |
                          PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
  return (mbi.State == MEM_COMMIT) && !!(mbi.Protect & exec_prot);
}

inline bool IsBadProtect(MEMORY_BASIC_INFORMATION const& mbi)
  HADESMEM_DETAIL_NOEXCEPT
{
  return !!(mbi.Protect & (PAGE_GUARD | PAGE_NOCACHE | PAGE_WRITECOMBINE));
}

inline bool IsGuard(MEMORY_BASIC_INFORMATION const& mbi)
  HADESMEM_DETAIL_NOEXCEPT
{
  return !!(mbi.Protect & PAGE_GUARD);
}

inline bool IsNoCache(MEMORY_BASIC_INFORMATION const& mbi)
  HADESMEM_DETAIL_NOEXCEPT
{
  return !!(mbi.Protect & PAGE_NOCACHE);
}

inline bool IsWriteCombine(MEMORY_BASIC_INFORMATION const& mbi)
  HADESMEM_DETAIL_NOEXCEPT
{
  return !!(mbi.Protect & PAGE_WRITECOMBINE);
}
}
}

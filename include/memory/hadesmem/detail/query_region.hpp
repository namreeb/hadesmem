// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>

#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
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
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("VirtualQueryEx failed.") << 
      ErrorCodeWinLast(last_error));
  }
  
  return mbi;
}

inline bool CanRead(MEMORY_BASIC_INFORMATION const& mbi) HADESMEM_NOEXCEPT
{
  return (mbi.State != MEM_RESERVE) && 
    ((mbi.Protect & PAGE_EXECUTE_READ) == PAGE_EXECUTE_READ || 
    (mbi.Protect & PAGE_EXECUTE_READWRITE) == PAGE_EXECUTE_READWRITE || 
    (mbi.Protect & PAGE_EXECUTE_WRITECOPY) == PAGE_EXECUTE_WRITECOPY || 
    (mbi.Protect & PAGE_READONLY) == PAGE_READONLY || 
    (mbi.Protect & PAGE_READWRITE) == PAGE_READWRITE || 
    (mbi.Protect & PAGE_WRITECOPY) == PAGE_WRITECOPY);
}

inline bool CanWrite(MEMORY_BASIC_INFORMATION const& mbi) HADESMEM_NOEXCEPT
{
  return (mbi.State != MEM_RESERVE) && 
    ((mbi.Protect & PAGE_EXECUTE_READWRITE) == PAGE_EXECUTE_READWRITE || 
    (mbi.Protect & PAGE_EXECUTE_WRITECOPY) == PAGE_EXECUTE_WRITECOPY || 
    (mbi.Protect & PAGE_READWRITE) == PAGE_READWRITE || 
    (mbi.Protect & PAGE_WRITECOPY) == PAGE_WRITECOPY);
}

inline bool CanExecute(MEMORY_BASIC_INFORMATION const& mbi) HADESMEM_NOEXCEPT
{
  return (mbi.State != MEM_RESERVE) && 
    ((mbi.Protect & PAGE_EXECUTE) == PAGE_EXECUTE || 
    (mbi.Protect & PAGE_EXECUTE_READ) == PAGE_EXECUTE_READ || 
    (mbi.Protect & PAGE_EXECUTE_READWRITE) == PAGE_EXECUTE_READWRITE || 
    (mbi.Protect & PAGE_EXECUTE_WRITECOPY) == PAGE_EXECUTE_WRITECOPY);
}

inline bool IsGuard(MEMORY_BASIC_INFORMATION const& mbi) HADESMEM_NOEXCEPT
{
  return (mbi.Protect & PAGE_GUARD) == PAGE_GUARD;
}

}

}

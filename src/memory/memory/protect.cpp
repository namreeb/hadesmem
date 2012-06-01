// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/protect.hpp"

#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"

namespace hadesmem
{

namespace detail
{

MEMORY_BASIC_INFORMATION Query(Process const& process, LPCVOID address)
{
  MEMORY_BASIC_INFORMATION mbi;
  ZeroMemory(&mbi, sizeof(mbi));
  if (::VirtualQueryEx(process.GetHandle(), address, &mbi, sizeof(mbi)) != 
    sizeof(mbi))
  {
    DWORD const last_error = GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("VirtualQueryEx failed.") << 
      ErrorCodeWinLast(last_error));
  }
  
  return mbi;
}

}

bool CanRead(Process const& process, LPCVOID address)
{
  MEMORY_BASIC_INFORMATION mbi = detail::Query(process, address);
  
  return (mbi.State != MEM_RESERVE) && 
    ((mbi.Protect & PAGE_EXECUTE_READ) == PAGE_EXECUTE_READ || 
    (mbi.Protect & PAGE_EXECUTE_READWRITE) == PAGE_EXECUTE_READWRITE || 
    (mbi.Protect & PAGE_EXECUTE_WRITECOPY) == PAGE_EXECUTE_WRITECOPY || 
    (mbi.Protect & PAGE_READONLY) == PAGE_READONLY || 
    (mbi.Protect & PAGE_READWRITE) == PAGE_READWRITE || 
    (mbi.Protect & PAGE_WRITECOPY) == PAGE_WRITECOPY);
}

bool CanWrite(Process const& process, LPCVOID address)
{
  MEMORY_BASIC_INFORMATION mbi = detail::Query(process, address);
  
  return (mbi.State != MEM_RESERVE) && 
    ((mbi.Protect & PAGE_EXECUTE_READWRITE) == PAGE_EXECUTE_READWRITE || 
    (mbi.Protect & PAGE_EXECUTE_WRITECOPY) == PAGE_EXECUTE_WRITECOPY || 
    (mbi.Protect & PAGE_READWRITE) == PAGE_READWRITE || 
    (mbi.Protect & PAGE_WRITECOPY) == PAGE_WRITECOPY);
}

bool CanExecute(Process const& process, LPCVOID address)
{
  MEMORY_BASIC_INFORMATION mbi = detail::Query(process, address);
  
  return (mbi.State != MEM_RESERVE) && 
    ((mbi.Protect & PAGE_EXECUTE) == PAGE_EXECUTE || 
    (mbi.Protect & PAGE_EXECUTE_READ) == PAGE_EXECUTE_READ || 
    (mbi.Protect & PAGE_EXECUTE_READWRITE) == PAGE_EXECUTE_READWRITE || 
    (mbi.Protect & PAGE_EXECUTE_WRITECOPY) == PAGE_EXECUTE_WRITECOPY);
}

bool IsGuard(Process const& process, LPCVOID address)
{
  MEMORY_BASIC_INFORMATION mbi = detail::Query(process, address);
  
  return (mbi.Protect & PAGE_GUARD) == PAGE_GUARD;
}

DWORD Protect(Process const& process, LPVOID address, DWORD protect)
{
  MEMORY_BASIC_INFORMATION mbi = detail::Query(process, address);
  
  DWORD old_protect = 0;
  if (!::VirtualProtectEx(process.GetHandle(), mbi.BaseAddress, mbi.RegionSize, 
    protect, &old_protect))
  {
    DWORD const last_error = GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("VirtualProtectEx failed.") << 
      ErrorCodeWinLast(last_error));
  }
  
  return old_protect;
}

}

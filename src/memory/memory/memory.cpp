// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/memory.hpp"

#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"

namespace hadesmem
{

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

PVOID Alloc(Process const& process, SIZE_T size)
{
  PVOID const address = ::VirtualAllocEx(process.GetHandle(), nullptr, 
    size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
  if (!address)
  {
    DWORD const last_error = GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("VirtualAllocEx failed.") << 
      ErrorCodeWinLast(last_error));
  }

  return address;
}

void Free(Process const& process, LPVOID address)
{
  if (!::VirtualFreeEx(process.GetHandle(), address, 0, MEM_RELEASE))
  {
    DWORD const last_error = GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("VirtualFreeEx failed.") << 
      ErrorCodeWinLast(last_error));
  }
}

void FlushInstructionCache(Process const& process, LPCVOID address, SIZE_T size)
{
  if (!::FlushInstructionCache(process.GetHandle(), address, size))
  {
    DWORD const last_error = GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("FlushInstructionCache failed.") << 
      ErrorCodeWinLast(last_error));
  }
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

void Read(Process const& process, LPVOID address, LPVOID out, std::size_t out_size)
{
 if (IsGuard(process, address))
  {
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Attempt to read from guard page."));
  }
  
  bool const can_read = CanRead(process, address);

  DWORD old_protect = 0;
  if (!can_read)
  {
    old_protect = Protect(process, address, PAGE_EXECUTE_READWRITE);
  }
  
  SIZE_T bytes_read = 0;
  if (!::ReadProcessMemory(process.GetHandle(), address, out, 
    out_size, &bytes_read) || bytes_read != out_size)
  {
    if (!can_read)
    {
      try
      {
        Protect(process, address, old_protect);
      }
      catch (std::exception const& /*e*/)
      { }
    }

    DWORD const last_error = GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Could not read process memory.") << 
      ErrorCodeWinLast(last_error));
  }
  
  if (!can_read)
  {
    Protect(process, address, old_protect);
  }
}

void Write(Process const& process, PVOID address, LPCVOID in, std::size_t in_size)
{
  if (IsGuard(process, address))
  {
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Attempt to write to guard page."));
  }
  
  bool const can_write = CanWrite(process, address);
  
  DWORD old_protect = 0;
  if (!can_write)
  {
    old_protect = Protect(process, address, PAGE_EXECUTE_READWRITE);
  }
  
  SIZE_T bytes_written = 0;
  if (!WriteProcessMemory(process.GetHandle(), address, in, 
    in_size, &bytes_written) || bytes_written != in_size)
  {
    if (!can_write)
    {
      try
      {
        Protect(process, address, old_protect);
      }
      catch (std::exception const& /*e*/)
      { }
    }
    
    DWORD const last_error = GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Could not write process memory.") << 
      ErrorCodeWinLast(last_error));
  }
  
  if (!can_write)
  {
    Protect(process, address, old_protect);
  }
}

}

}

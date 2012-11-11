// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "initialize.hpp"

#include <ctime>
#include <random>
#include <cassert>
#include <iostream>

#include <windows.h>
// Required for ::_CrtSetDbgFlag.
// Does not compile under GCC.
#if !defined(HADESMEM_GCC)
// Macro unused under Clang.
#if !defined(HADESMEM_CLANG)
#define _CRTDBG_MAP_ALLOC
#endif // #if !defined(HADESMEM_CLANG)
#include <stdlib.h>
#include <crtdbg.h>
#endif // #if !defined(HADESMEM_GCC)

#include <hadesmem/error.hpp>

#ifndef PROCESS_CALLBACK_FILTER_ENABLED
#define PROCESS_CALLBACK_FILTER_ENABLED 0x1
#endif // #ifndef PROCESS_CALLBACK_FILTER_ENABLED

void DisableUserModeCallbackExceptionFilter()
{
  HMODULE const k32_mod = ::GetModuleHandle(L"kernel32");
  if (!k32_mod)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(hadesmem::Error() << 
      hadesmem::ErrorString("GetModuleHandle failed.") << 
      hadesmem::ErrorCodeWinLast(last_error));
  }

  typedef BOOL (NTAPI *GetProcessUserModeExceptionPolicyPtr)(LPDWORD lpFlags);
  typedef BOOL (NTAPI *SetProcessUserModeExceptionPolicyPtr)(DWORD dwFlags);

  // These APIs are not available by default until Windows 7 SP1, so they 
  // must be called dynamically.
  auto const get_policy = 
    reinterpret_cast<GetProcessUserModeExceptionPolicyPtr>(
    ::GetProcAddress(k32_mod, "GetProcessUserModeExceptionPolicy"));
  auto const set_policy = 
    reinterpret_cast<SetProcessUserModeExceptionPolicyPtr>(
    ::GetProcAddress(k32_mod, "SetProcessUserModeExceptionPolicy"));

  DWORD flags;
  if (get_policy && set_policy && get_policy(&flags))
  {
    set_policy(flags & static_cast<DWORD>(~PROCESS_CALLBACK_FILTER_ENABLED));
  }
}

void EnableCrtDebugFlags()
{
  // Only enable in debug mode.
#if defined(_DEBUG)
  // Does not compile under GCC.
#if !defined(HADESMEM_GCC)
  int dbg_flags = ::_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
  dbg_flags |= _CRTDBG_ALLOC_MEM_DF;
  dbg_flags |= _CRTDBG_DELAY_FREE_MEM_DF;
  dbg_flags |= _CRTDBG_LEAK_CHECK_DF;
  ::_CrtSetDbgFlag(dbg_flags);
#endif // #if !defined(HADESMEM_GCC)
#endif // #if defined(_DEBUG)
}

void EnableTerminationOnHeapCorruption()
{
  if (!::HeapSetInformation(::GetProcessHeap(), 
    HeapEnableTerminationOnCorruption, nullptr, 0))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(hadesmem::Error() << 
      hadesmem::ErrorString("HeapSetInformation failed.") << 
      hadesmem::ErrorCodeWinLast(last_error));
  }
}

void EnableBottomUpRand()
{
  // This is a defense-in-depth measure, so the time should be sufficient 
  // entropy in all but the rarest cases.
  std::mt19937 rand(static_cast<unsigned int>(std::time(nullptr)));
  std::uniform_int_distribution<unsigned int> uint_dist(0, 256);
  unsigned int const num_allocs = uint_dist(rand);
  for (unsigned int i = 0; i != num_allocs; ++i)
  {
    // Reserve memory only, as committing it would be a waste.
    DWORD const kAllocSize64K = 64 * 1024;
    if (!::VirtualAlloc(NULL, kAllocSize64K, MEM_RESERVE, PAGE_NOACCESS))
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_THROW_EXCEPTION(hadesmem::Error() << 
        hadesmem::ErrorString("VirtualAlloc failed.") << 
        hadesmem::ErrorCodeWinLast(last_error));
    }
  }
}

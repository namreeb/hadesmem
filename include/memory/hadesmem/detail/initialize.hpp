// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <ctime>
#include <locale>
#include <random>
#include <iostream>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/locale.hpp>
#include <boost/filesystem.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
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
#include <hadesmem/module.hpp>
#include <hadesmem/detail/assert.hpp>

#ifndef PROCESS_CALLBACK_FILTER_ENABLED
#define PROCESS_CALLBACK_FILTER_ENABLED 0x1UL
#endif // #ifndef PROCESS_CALLBACK_FILTER_ENABLED

#include <windows.h>

namespace hadesmem
{

namespace detail
{
  
struct ProcessMitigationPolicy
{
  typedef enum _PROCESS_MITIGATION_POLICY
  {  
    ProcessDEPPolicy,  
    ProcessASLRPolicy,  
    ProcessReserved1MitigationPolicy,  
    ProcessStrictHandleCheckPolicy,  
    ProcessSystemCallDisablePolicy,  
    ProcessMitigationOptionsMask,  
    ProcessExtensionPointDisablePolicy,  
    MaxProcessMitigationPolicy  
  } PROCESS_MITIGATION_POLICY, *PPROCESS_MITIGATION_POLICY;  

  typedef struct _PROCESS_MITIGATION_ASLR_POLICY
  {
    union
    {
      ULONG Flags;
      struct
      {
        ULONG EnableBottomUpRandomization : 1;
        ULONG EnableForceRelocateImages : 1;
        ULONG EnableHighEntropy : 1;
        ULONG DisallowStrippedImages : 1;
        ULONG ReservedFlags : 28;
      } Bits;
    };
  } PROCESS_MITIGATION_ASLR_POLICY, *PPROCESS_MITIGATION_ASLR_POLICY;

  typedef struct _PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY
  {
    union
    {
      ULONG Flags;
      struct
      {
        ULONG RaiseExceptionOnInvalidHandleReference : 1;
        ULONG HandleExceptionsPermanentlyEnabled : 1;
        ULONG ReservedFlags : 30;
      } Bits;
    };
  } PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY, *PPROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY;
};

// Disables the user-mode callback exception filter present in 64-bit versions 
// of Windows. Microsoft Support article KB976038 (http://bit.ly/8yZMvw).
inline void DisableUserModeCallbackExceptionFilter()
{
  HMODULE const k32_mod = ::GetModuleHandle(L"kernel32");
  if (!k32_mod)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("GetModuleHandle failed.") << 
      ErrorCodeWinLast(last_error));
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
    set_policy(flags & ~PROCESS_CALLBACK_FILTER_ENABLED);
  }
}

// Enables CRT debug flags for memory leak detection in debug builds.
inline void EnableCrtDebugFlags()
{
  // Only enable in debug mode.
#if defined(_DEBUG)
  // Does not compile under GCC.
#if !defined(HADESMEM_GCC)
  int dbg_flags = ::_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
  dbg_flags |= _CRTDBG_ALLOC_MEM_DF;
  dbg_flags |= _CRTDBG_LEAK_CHECK_DF;
  ::_CrtSetDbgFlag(dbg_flags);
#endif // #if !defined(HADESMEM_GCC)
#endif // #if defined(_DEBUG)
}

// Forces termination of the application if heap corruption is detected.
// Recommended as per Microsoft article "Windows ISV Software Security 
// Defenses" (http://bit.ly/i5yLdM).
inline void EnableTerminationOnHeapCorruption()
{
  if (!::HeapSetInformation(::GetProcessHeap(), 
    HeapEnableTerminationOnCorruption, nullptr, 0))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("HeapSetInformation failed.") << 
      ErrorCodeWinLast(last_error));
  }
}

// Custom 'bottom up randomization' implementation similar to that of EMET.
// Modified version of code by Didier Stevens (http://bit.ly/qUhc9K).
inline void EnableBottomUpRand()
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
      HADESMEM_THROW_EXCEPTION(Error() << 
        ErrorString("VirtualAlloc failed.") << 
        ErrorCodeWinLast(last_error));
    }
  }
}

// Enables extra process mitigation policies. Currently hardens ASLR and 
// handle policies.
inline void EnableMitigationPolicies()
{
  HMODULE const kernel32_mod = ::GetModuleHandle(L"kernel32.dll");
  if (!kernel32_mod)
  {
    DWORD const last_error = ::GetLastError();
      HADESMEM_THROW_EXCEPTION(Error() << 
        ErrorString("GetModuleHandle failed.") << 
        ErrorCodeWinLast(last_error));
  }
  
  // These APIs are not available by default until Windows 8, so they 
  // must be called dynamically.
  typedef BOOL (WINAPI * GetProcessMitigationPolicyPtr) (
    HANDLE hProcess, 
    ProcessMitigationPolicy::PROCESS_MITIGATION_POLICY MitigationPolicy, 
    PVOID lpBuffer, SIZE_T dwLength);
  auto const get_mitigation_policy = 
    reinterpret_cast<GetProcessMitigationPolicyPtr>(::GetProcAddress(
    kernel32_mod, "GetProcessMitigationPolicy"));
  if (!get_mitigation_policy)
  {
    return;
  }

  typedef BOOL (WINAPI * SetProcessMitigationPolicyPtr) (
    ProcessMitigationPolicy::PROCESS_MITIGATION_POLICY MitigationPolicy, 
    PVOID lpBuffer, SIZE_T dwLength);
  auto const set_mitigation_policy = 
    reinterpret_cast<SetProcessMitigationPolicyPtr>(::GetProcAddress(
    kernel32_mod, "SetProcessMitigationPolicy"));
  if (!set_mitigation_policy)
  {
    return;
  }

  ProcessMitigationPolicy::PROCESS_MITIGATION_ASLR_POLICY aslr_policy = 
    { { 0 } };
  if (!get_mitigation_policy(::GetCurrentProcess(), 
    ProcessMitigationPolicy::ProcessASLRPolicy, &aslr_policy, 
    sizeof(aslr_policy)))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("GetProcessMitigationPolicy failed.") << 
      ErrorCodeWinLast(last_error));
  }

  aslr_policy.Bits.EnableForceRelocateImages = 1;
  aslr_policy.Bits.DisallowStrippedImages = 1;
  if (!set_mitigation_policy(ProcessMitigationPolicy::ProcessASLRPolicy, 
    &aslr_policy, sizeof(aslr_policy)))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("SetProcessMitigationPolicy failed.") << 
      ErrorCodeWinLast(last_error));
  }

  ProcessMitigationPolicy::PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY 
    strict_handle_check_policy = { { 0 } };
  if (!get_mitigation_policy(::GetCurrentProcess(), 
    ProcessMitigationPolicy::ProcessStrictHandleCheckPolicy, 
    &strict_handle_check_policy, sizeof(strict_handle_check_policy)))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("GetProcessMitigationPolicy failed.") << 
      ErrorCodeWinLast(last_error));
  }

  strict_handle_check_policy.Bits.RaiseExceptionOnInvalidHandleReference = 1;
  strict_handle_check_policy.Bits.HandleExceptionsPermanentlyEnabled = 1;
  if (!set_mitigation_policy(
    ProcessMitigationPolicy::ProcessStrictHandleCheckPolicy, 
    &strict_handle_check_policy, sizeof(strict_handle_check_policy)))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("SetProcessMitigationPolicy failed.") << 
      ErrorCodeWinLast(last_error));
  }
}

// Sets the global locale, and imbues all existing static streams with the 
// new locale (including 3rd party libraries like Boost.Filesystem).
inline std::locale ImbueAll(std::locale const& locale)
{
  auto const old_loc = std::locale::global(locale);

  std::cin.imbue(locale);
  std::cout.imbue(locale);
  std::cerr.imbue(locale);
  std::clog.imbue(locale);

  std::wcin.imbue(locale);
  std::wcout.imbue(locale);
  std::wcerr.imbue(locale);
  std::wclog.imbue(locale);

  boost::filesystem::path::imbue(locale);

  // According to comments in the Boost.Locale examples, this is needed 
  // to prevent the C standard library performing string conversions 
  // instead of the C++ standard library on some platforms. Unfortunately, 
  // it is not specified which platforms this applies to, so I'm unsure 
  // if this is ever relevant to Windows, but it's better safe than sorry.
  std::ios_base::sync_with_stdio(false); 

  return old_loc;
}

// Generates a new UTF-8 based locale object, sets the global locale, and 
// imbues all known static streams.
inline std::locale ImbueAllDefault()
{
  // Use the Windows API backend to (hopefully) provide consistent behaviour 
  // across different compilers and standard library implementations (as 
  // opposed to the standard library backend).
  auto backend = boost::locale::localization_backend_manager::global();
  backend.select("winapi"); 

  boost::locale::generator gen(backend);

  std::locale const locale = gen("");

  // Ensure the locale uses a UTF-8 backend. 
  HADESMEM_ASSERT(std::use_facet<boost::locale::info>(locale).utf8());

  return ImbueAll(locale);
}

// Perform all initialization tasks.
inline void InitializeAll()
{
  DisableUserModeCallbackExceptionFilter();
  EnableCrtDebugFlags();
  EnableTerminationOnHeapCorruption();
  EnableBottomUpRand();
  // Enabling mitigation policies under MinGW-w64 causes a crash.
  // TODO: Investigate why this is failing under MinGW-w64 and fix.
#if !defined(HADESMEM_CLANG) && !defined(HADESMEM_GCC)
  EnableMitigationPolicies();
#endif
  ImbueAllDefault();
}

}

}

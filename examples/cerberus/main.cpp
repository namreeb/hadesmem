// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/process.hpp>

#include "file.hpp"
#include "process.hpp"

// WARNING! Most of this is untested, it's for expository and testing
// purposes only.
// TODO: Write a test app to exercise all the hooked APIs and information
// classes.

hadesmem::Process& GetThisProcess()
{
  static hadesmem::Process process(::GetCurrentProcessId());
  return process;
}

extern "C" HADESMEM_DETAIL_DLLEXPORT DWORD_PTR Load() HADESMEM_DETAIL_NOEXCEPT
{
  try
  {
    DetourNtQuerySystemInformation();
    DetourNtQueryDirectoryFile();
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }

  return 0;
}

// TODO: Safe unhooking and unloading.
extern "C" HADESMEM_DETAIL_DLLEXPORT DWORD_PTR Free() HADESMEM_DETAIL_NOEXCEPT
{
  return 0;
}

BOOL WINAPI
  DllMain(HINSTANCE /*instance*/, DWORD /*reason*/, LPVOID /*reserved*/)
{
  return TRUE;
}

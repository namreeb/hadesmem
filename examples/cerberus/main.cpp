// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/process.hpp>

#include "d3d11.hpp"
#include "file.hpp"
#include "module.hpp"
#include "process.hpp"

// WARNING! Most of this is untested, it's for expository and testing
// purposes only.

hadesmem::Process& GetThisProcess()
{
  static hadesmem::Process process{::GetCurrentProcessId()};
  return process;
}

extern "C" HADESMEM_DETAIL_DLLEXPORT DWORD_PTR Load() HADESMEM_DETAIL_NOEXCEPT
{
  try
  {
    DetourNtQuerySystemInformation();
    DetourNtQueryDirectoryFile();
    DetourNtMapViewOfSection();

    // If these aren't currently loaded they will be hooked on load via the
    // NtMapViewOfSection hook.
    DetourD3D11(nullptr);
    DetourDXGI(nullptr);
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }

  return 0;
}

extern "C" HADESMEM_DETAIL_DLLEXPORT DWORD_PTR Free() HADESMEM_DETAIL_NOEXCEPT
{
  try
  {
    UndetourNtQuerySystemInformation();
    UndetourNtQueryDirectoryFile();
    UndetourNtMapViewOfSection();
    UndetourDXGI();
    UndetourD3D11();
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }

  return 0;
}

BOOL WINAPI
  DllMain(HINSTANCE /*instance*/, DWORD /*reason*/, LPVOID /*reserved*/)
{
  return TRUE;
}

// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/error.hpp>

#include <cerberus/plugin.hpp>

#include "gui.hpp"

extern "C" HADESMEM_DETAIL_DLLEXPORT DWORD_PTR LoadPlugin(
  hadesmem::cerberus::PluginInterface* cerberus) HADESMEM_DETAIL_NOEXCEPT
{
  try
  {
    HADESMEM_DETAIL_TRACE_A("Initializing.");

    InitializeGui(cerberus);

    return 0;
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);

    return 1;
  }
}

extern "C" HADESMEM_DETAIL_DLLEXPORT DWORD_PTR UnloadPlugin(
  hadesmem::cerberus::PluginInterface* cerberus) HADESMEM_DETAIL_NOEXCEPT
{
  try
  {
    HADESMEM_DETAIL_TRACE_A("Cleaning up.");

    CleanupGui(cerberus);

    return 0;
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);

    return 1;
  }
}

BOOL WINAPI DllMain(HINSTANCE /*instance*/,
                    DWORD /*reason*/,
                    LPVOID /*reserved*/) HADESMEM_DETAIL_NOEXCEPT
{
  return TRUE;
}

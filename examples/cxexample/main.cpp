// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/error.hpp>

#include <cerberus/plugin.hpp>

#include "gui.hpp"

extern "C" HADESMEM_DETAIL_DLLEXPORT void LoadPlugin(
  hadesmem::cerberus::PluginInterface* cerberus) HADESMEM_DETAIL_NOEXCEPT
{
  HADESMEM_DETAIL_TRACE_A("Initializing.");

  InitializeGui(cerberus);
}

extern "C" HADESMEM_DETAIL_DLLEXPORT void UnloadPlugin(
  hadesmem::cerberus::PluginInterface* cerberus) HADESMEM_DETAIL_NOEXCEPT
{
  HADESMEM_DETAIL_TRACE_A("Cleaning up.");

  CleanupGui(cerberus);
}

BOOL WINAPI DllMain(HINSTANCE /*instance*/,
                    DWORD /*reason*/,
                    LPVOID /*reserved*/) HADESMEM_DETAIL_NOEXCEPT
{
  return TRUE;
}

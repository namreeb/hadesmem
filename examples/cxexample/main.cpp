// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/error.hpp>

#include <cerberus/plugin.hpp>

#include "gui.hpp"

extern "C" __declspec(dllexport) void LoadPlugin(
  hadesmem::cerberus::PluginInterface* cerberus) noexcept
{
  HADESMEM_DETAIL_TRACE_A("Initializing.");

  InitializeGui(cerberus);
}

extern "C" __declspec(dllexport) void UnloadPlugin(
  hadesmem::cerberus::PluginInterface* cerberus) noexcept
{
  HADESMEM_DETAIL_TRACE_A("Cleaning up.");

  CleanupGui(cerberus);
}

BOOL WINAPI DllMain(HINSTANCE /*instance*/,
                    DWORD /*reason*/,
                    LPVOID /*reserved*/) noexcept
{
  return TRUE;
}

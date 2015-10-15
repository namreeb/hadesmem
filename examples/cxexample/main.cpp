// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/error.hpp>

#include <cerberus/plugin.hpp>
#include <cerberus/d3d9.hpp>

#include "gui.hpp"

namespace
{
std::size_t g_on_frame_callback_id = static_cast<std::size_t>(-1);

void OnFrameCallback(IDirect3DDevice9* /*device*/)
{
}
}

extern "C" __declspec(dllexport) void LoadPlugin(
  hadesmem::cerberus::PluginInterface* cerberus) noexcept
{
  HADESMEM_DETAIL_TRACE_A("Initializing.");

  g_on_frame_callback_id =
    cerberus->GetD3D9Interface()->RegisterOnFrame(&OnFrameCallback);

  InitializeGui(cerberus);
}

extern "C" __declspec(dllexport) void UnloadPlugin(
  hadesmem::cerberus::PluginInterface* cerberus) noexcept
{
  HADESMEM_DETAIL_TRACE_A("Cleaning up.");

  cerberus->GetD3D9Interface()->UnregisterOnFrame(g_on_frame_callback_id);

  CleanupGui(cerberus);
}

BOOL WINAPI DllMain(HINSTANCE /*instance*/,
                    DWORD /*reason*/,
                    LPVOID /*reserved*/) noexcept
{
  return TRUE;
}

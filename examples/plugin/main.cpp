// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/error.hpp>

#include "../cerberus/plugin.hpp"

namespace
{
std::size_t g_on_frame_callback_id{};
}

extern "C" HADESMEM_DETAIL_DLLEXPORT DWORD_PTR
  LoadPlugin(hadesmem::cerberus::PluginInterface* cerberus)
  HADESMEM_DETAIL_NOEXCEPT
{
  try
  {
    HADESMEM_DETAIL_TRACE_A("Initializing.");

    auto const on_frame_callback = [](IDXGISwapChain* /*swap_chain*/,
                                      ID3D11Device* /*device*/,
                                      ID3D11DeviceContext* /*device_context*/)
    { HADESMEM_DETAIL_TRACE_A("Got an OnFrame event."); };
    g_on_frame_callback_id =
      cerberus->GetD3D11Interface()->RegisterOnFrameCallback(on_frame_callback);

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

extern "C" HADESMEM_DETAIL_DLLEXPORT DWORD_PTR
  UnloadPlugin(hadesmem::cerberus::PluginInterface* cerberus)
  HADESMEM_DETAIL_NOEXCEPT
{
  try
  {
    HADESMEM_DETAIL_TRACE_A("Cleaning up.");

    cerberus->GetD3D11Interface()->UnregisterOnFrameCallback(
      g_on_frame_callback_id);
    g_on_frame_callback_id = static_cast<std::uint32_t>(-1);

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

BOOL WINAPI
  DllMain(HINSTANCE /*instance*/, DWORD /*reason*/, LPVOID /*reserved*/)
  HADESMEM_DETAIL_NOEXCEPT
{
  return TRUE;
}

// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include <thread>

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/self_path.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/error.hpp>

#include "../cerberus/plugin.hpp"

#include "window.hpp"

namespace
{
std::size_t g_on_frame_callback_id{};

std::unique_ptr<std::thread> g_window_thread{};
}

extern "C" HADESMEM_DETAIL_DLLEXPORT DWORD_PTR
  LoadPlugin(hadesmem::cerberus::PluginInterface* cerberus)
  HADESMEM_DETAIL_NOEXCEPT
{
  try
  {
    HADESMEM_DETAIL_TRACE_A("Initializing.");

    auto const on_frame_callback = [](IDXGISwapChain* /*swap_chain*/)
    { HADESMEM_DETAIL_TRACE_A("Got an OnFrame event."); };
    g_on_frame_callback_id =
      cerberus->GetD3D11Interface()->RegisterOnFrameCallback(on_frame_callback);

    g_window_thread.reset(new std::thread(WindowThread,
                                          hadesmem::detail::GetHandleToSelf(),
                                          nullptr,
                                          nullptr,
                                          SW_SHOW));
    g_window_thread->detach();

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

    auto const hwnd = GetWindowHandle();
    if (hwnd && ::SendMessageW(hwnd, WM_CLOSE, 0, 0))
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"SendMessageW failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }

    auto const thread = GetThreadHandle();
    if (thread)
    {
      DWORD const wait_result = ::WaitForSingleObject(thread, INFINITE);
      if (wait_result != WAIT_OBJECT_0)
      {
        DWORD const last_error = ::GetLastError();
        HADESMEM_DETAIL_THROW_EXCEPTION(
          hadesmem::Error{}
          << hadesmem::ErrorString{"WaitForSingleObject failed."}
          << hadesmem::ErrorCodeWinLast{last_error}
          << hadesmem::ErrorCodeWinOther(wait_result));
      }
    }

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

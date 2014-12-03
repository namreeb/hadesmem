// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "main.hpp"

#include <algorithm>
#include <functional>
#include <mutex>

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/self_path.hpp>
#include <hadesmem/detail/region_alloc_size.hpp>
#include <hadesmem/detail/thread_aux.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/thread.hpp>
#include <hadesmem/thread_entry.hpp>
#include <hadesmem/thread_helpers.hpp>
#include <hadesmem/thread_list.hpp>

#include "ant_tweak_bar.hpp"
#include "cursor.hpp"
#include "d3d9.hpp"
#include "d3d10.hpp"
#include "d3d11.hpp"
#include "direct_input.hpp"
#include "dxgi.hpp"
#include "exception.hpp"
#include "input.hpp"
#include "module.hpp"
#include "opengl.hpp"
#include "plugin.hpp"
#include "process.hpp"
#include "render.hpp"
#include "window.hpp"

// WARNING! Most of this is untested, it's for expository and testing
// purposes only.

namespace
{
// This is a nasty hack to call any APIs which may be called from a static
// destructor. We want to ensure that we call it nice and early, so it's not
// called after we load our plugins, because otherwise it will be destructed
// before the plugin's are automatically unloaded via the static destructor of
// the plugin list, and when plugins try to unregister their callbacks (or
// whatever they're doing) they will go boom. This is a nasty workaround, but
// it's guaranteed by the standard to work, because we always use function local
// statics which are guaranteed to be destructed in a deterministic order.
void UseAllStatics()
{
  hadesmem::cerberus::GetThisProcess();

  auto& module = hadesmem::cerberus::GetModuleInterface();
  auto& d3d9 = hadesmem::cerberus::GetD3D9Interface();
  auto& dxgi = hadesmem::cerberus::GetDXGIInterface();
  auto& render = hadesmem::cerberus::GetRenderInterface();
  auto& ant_tweak_bar = hadesmem::cerberus::GetAntTweakBarInterface();
  auto& window = hadesmem::cerberus::GetWindowInterface();
  auto& direct_input = hadesmem::cerberus::GetDirectInputInterface();
  auto& cursor = hadesmem::cerberus::GetCursorInterface();
  auto& input = hadesmem::cerberus::GetInputInterface();

  // Have to use 'real' callbacks rather than just passing in an empty
  // std::function object because we might not be the only thread running at the
  // moment and calling an empty function wrapper throws.

  auto const on_map_callback =
    [](HMODULE, std::wstring const&, std::wstring const&)
  {
  };
  auto const on_map_id = module.RegisterOnMap(on_map_callback);
  module.UnregisterOnMap(on_map_id);

  auto const on_unmap_callback = [](HMODULE)
  {
  };
  auto const on_unmap_id = module.RegisterOnUnmap(on_unmap_callback);
  module.UnregisterOnUnmap(on_unmap_id);

  auto const on_frame_callback_d3d9 = [](IDirect3DDevice9*)
  {
  };
  auto const on_frame_id_d3d9 = d3d9.RegisterOnFrame(on_frame_callback_d3d9);
  d3d9.UnregisterOnFrame(on_frame_id_d3d9);

  auto const on_frame_callback_dxgi = [](IDXGISwapChain*)
  {
  };
  auto const on_frame_id_dxgi = dxgi.RegisterOnFrame(on_frame_callback_dxgi);
  dxgi.UnregisterOnFrame(on_frame_id_dxgi);

  auto const on_tw_init = [](hadesmem::cerberus::AntTweakBarInterface*)
  {
  };
  auto const on_tw_init_id = ant_tweak_bar.RegisterOnInitialize(on_tw_init);
  ant_tweak_bar.UnregisterOnInitialize(on_tw_init_id);

  auto const on_tw_cleanup = [](hadesmem::cerberus::AntTweakBarInterface*)
  {
  };
  auto const on_tw_cleanup_id = ant_tweak_bar.RegisterOnCleanup(on_tw_cleanup);
  ant_tweak_bar.UnregisterOnCleanup(on_tw_cleanup_id);

  auto const on_frame = []()
  {
  };
  auto const on_frame_id = render.RegisterOnFrame(on_frame);
  render.UnregisterOnFrame(on_frame_id);

  auto const on_wnd_proc_msg = [](HWND, UINT, WPARAM, LPARAM, bool*)
  {
  };
  auto const on_wnd_proc_msg_id = window.RegisterOnWndProcMsg(on_wnd_proc_msg);
  window.UnregisterOnWndProcMsg(on_wnd_proc_msg_id);

  auto const on_set_cursor = [](HCURSOR, bool*, HCURSOR*)
  {
  };
  auto const on_set_cursor_id = cursor.RegisterOnSetCursor(on_set_cursor);
  cursor.UnregisterOnSetCursor(on_set_cursor_id);

  auto const on_get_cursor_pos = [](LPPOINT, bool*)
  {
  };
  auto const on_get_cursor_pos_id =
    cursor.RegisterOnGetCursorPos(on_get_cursor_pos);
  cursor.UnregisterOnGetCursorPos(on_get_cursor_pos_id);

  auto const on_set_cursor_pos = [](int, int, bool*)
  {
  };
  auto const on_set_cursor_pos_id =
    cursor.RegisterOnSetCursorPos(on_set_cursor_pos);
  cursor.UnregisterOnSetCursorPos(on_set_cursor_pos_id);

  auto const on_show_cursor = [](BOOL, bool*, int*)
  {
  };
  auto const on_show_cursor_id = cursor.RegisterOnShowCursor(on_show_cursor);
  cursor.UnregisterOnShowCursor(on_show_cursor_id);

  auto const on_clip_cursor = [](RECT const*, bool*, BOOL*)
  {
  };
  auto const on_clip_cursor_id = cursor.RegisterOnClipCursor(on_clip_cursor);
  cursor.UnregisterOnClipCursor(on_clip_cursor_id);

  auto const on_get_clip_cursor = [](RECT*, bool*, BOOL*)
  {
  };
  auto const on_get_clip_cursor_id =
    cursor.RegisterOnGetClipCursor(on_get_clip_cursor);
  cursor.UnregisterOnGetClipCursor(on_get_clip_cursor_id);

  auto const on_direct_input = [](bool*)
  {
  };
  auto const on_direct_input_id =
    direct_input.RegisterOnDirectInput(on_direct_input);
  direct_input.UnregisterOnDirectInput(on_direct_input_id);

  auto const on_set_gui_visibility = [](bool, bool)
  {
  };
  auto const on_set_gui_visibility_id =
    input.RegisterOnSetGuiVisibility(on_set_gui_visibility);
  input.UnregisterOnSetGuiVisibility(on_set_gui_visibility_id);

  auto const on_input_queue_entry = [](HWND, UINT, WPARAM, LPARAM)
  {
  };
  auto const on_input_queue_entry_id =
    input.RegisterOnInputQueueEntry(on_input_queue_entry);
  input.UnregisterOnInputQueueEntry(on_input_queue_entry_id);
}

// Check whether any threads are currently executing code in our module. This
// does not check whether we are on the stack, but that should be handled by the
// ref counting done in all the hooks. This is not foolproof, but it's better
// than nothing and will reduce the potential danger window even further.
bool IsSafeToUnload()
{
  auto const& process = hadesmem::cerberus::GetThisProcess();
  auto const this_module =
    reinterpret_cast<std::uint8_t*>(hadesmem::detail::GetHandleToSelf());
  auto const this_module_size = hadesmem::detail::GetRegionAllocSize(
    process, reinterpret_cast<void const*>(this_module));

  bool safe = false;
  for (std::size_t retries = 5; retries && !safe; --retries)
  {
    hadesmem::SuspendedProcess suspend{process.GetId()};
    hadesmem::ThreadList threads{process.GetId()};

    auto const is_unsafe = [&](hadesmem::ThreadEntry const& thread_entry)
    {
      auto const id = thread_entry.GetId();
      return id != ::GetCurrentThreadId() &&
             hadesmem::detail::IsExecutingInRange(
               thread_entry, this_module, this_module + this_module_size);
    };

    safe = std::find_if(std::begin(threads), std::end(threads), is_unsafe) ==
           std::end(threads);
  }

  return safe;
}

bool& GetInititalizeFlag()
{
  static bool initialized = false;
  return initialized;
}

std::mutex& GetInitializeMutex()
{
  static std::mutex mutex;
  return mutex;
}
}

namespace hadesmem
{
namespace cerberus
{
Process& GetThisProcess()
{
  static Process process{::GetCurrentProcessId()};
  return process;
}
}
}

extern "C" HADESMEM_DETAIL_DLLEXPORT DWORD_PTR Load() HADESMEM_DETAIL_NOEXCEPT
{
  try
  {
    std::mutex& mutex = GetInitializeMutex();
    std::lock_guard<std::mutex> lock(mutex);

    bool& is_initialized = GetInititalizeFlag();
    if (is_initialized)
    {
      HADESMEM_DETAIL_TRACE_A("Already initialized. Bailing.");
      return 1;
    }

    is_initialized = true;

    UseAllStatics();

    // Support deferred hooking (via module load notifications).
    hadesmem::cerberus::InitializeD3D9();
    hadesmem::cerberus::InitializeD3D10();
    hadesmem::cerberus::InitializeD3D101();
    hadesmem::cerberus::InitializeD3D11();
    hadesmem::cerberus::InitializeDXGI();
    hadesmem::cerberus::InitializeDirectInput();
    hadesmem::cerberus::InitializeCursor();

    hadesmem::cerberus::DetourCreateProcessInternalW();
    hadesmem::cerberus::DetourNtMapViewOfSection();
    hadesmem::cerberus::DetourNtUnmapViewOfSection();
    hadesmem::cerberus::DetourRtlAddVectoredExceptionHandler();

    hadesmem::cerberus::DetourD3D9(nullptr);
    hadesmem::cerberus::DetourD3D10(nullptr);
    hadesmem::cerberus::DetourD3D101(nullptr);
    hadesmem::cerberus::DetourD3D11(nullptr);
    hadesmem::cerberus::DetourDXGI(nullptr);
    hadesmem::cerberus::DetourDirectInput8(nullptr);
    hadesmem::cerberus::DetourUser32ForCursor(nullptr);
    hadesmem::cerberus::DetourOpenGL32(nullptr);

    hadesmem::cerberus::InitializeRender();
    hadesmem::cerberus::InitializeInput();
    hadesmem::cerberus::InitializeAntTweakBar();

    hadesmem::cerberus::LoadPlugins();

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

extern "C" HADESMEM_DETAIL_DLLEXPORT DWORD_PTR Free() HADESMEM_DETAIL_NOEXCEPT
{
  try
  {
    std::mutex& mutex = GetInitializeMutex();
    std::lock_guard<std::mutex> lock(mutex);

    bool& is_initialized = GetInititalizeFlag();
    if (!is_initialized)
    {
      HADESMEM_DETAIL_TRACE_A("Already cleaned up. Bailing.");
      return 1;
    }

    is_initialized = false;

    hadesmem::cerberus::UndetourCreateProcessInternalW();
    hadesmem::cerberus::UndetourNtMapViewOfSection();
    hadesmem::cerberus::UndetourNtUnmapViewOfSection();
    hadesmem::cerberus::UndetourRtlAddVectoredExceptionHandler();

    hadesmem::cerberus::UndetourDXGI(true);
    hadesmem::cerberus::UndetourD3D11(true);
    hadesmem::cerberus::UndetourD3D101(true);
    hadesmem::cerberus::UndetourD3D10(true);
    hadesmem::cerberus::UndetourD3D9(true);
    hadesmem::cerberus::UndetourDirectInput8(true);
    hadesmem::cerberus::UndetourUser32ForCursor(true);
    hadesmem::cerberus::UndetourOpenGL32(true);

    hadesmem::cerberus::UnloadPlugins();

    if (!IsSafeToUnload())
    {
      return 2;
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

BOOL WINAPI DllMain(HINSTANCE /*instance*/,
                    DWORD /*reason*/,
                    LPVOID /*reserved*/) HADESMEM_DETAIL_NOEXCEPT
{
  return TRUE;
}

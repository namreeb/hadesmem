// Copyright (C) 2010-2015 Joshua Boyce
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
#include "config.hpp"
#include "chaiscript.hpp"
#include "cursor.hpp"
#include "d3d9.hpp"
#include "direct_input.hpp"
#include "driver.hpp"
#include "dxgi.hpp"
#include "exception.hpp"
#include "helpers.hpp"
#include "imgui.hpp"
#include "imgui_log.hpp"
#include "input.hpp"
#include "module.hpp"
#include "opengl.hpp"
#include "plugin.hpp"
#include "process.hpp"
#include "raw_input.hpp"
#include "render.hpp"
#include "render_helper.hpp"
#include "service.hpp"
#include "window.hpp"

// WARNING! Most of this is untested, it's for expository and testing
// purposes only.

// TODO: Clean up this entire project.

// TODO: In API hooks where we allow callbacks to block calling the trampoline
// and fail the call, also ensure we use SetLastError to set a reasonable error
// code (or request one from the blocking callback).

// TODO: Investigate Uplay, Origin, Overwolf, etc. compatibility issues (both
// client and overlay).

// TODO: Add networking hooks and callbacks.

// TODO: All API hook callbacks should have a "retval" arg to compliment the
// "handled" arg. Ensure it has a sensible default because it will be unused in
// a lot of cases.

// TODO: Review the restrictions on DllMain and ensure that none of the static
// constructors or destructors are in violation.

// TODO: Split up hooking code from implementation logic. e.g. The generic
// process detour logic should be separated from the code which is reacting to
// the hook (injecting into the new proc etc.).

// TODO: Create a WineDB style spreadsheet to track game/app compatibility.
// Ensure we test in-game properly, not just at the menu, and that we test
// everything properly (basic rendering, basic input, fullscreen and windowed,
// changing resolution, changing input modes, tabbing out and back in when GUI
// is visible, etc.).

// TODO: Hook NTDLL instead of USER32 where possible.

// TODO: Write a test app which is AppVerifier clean, exercises all major
// functionality, etc. so we can test properly.

// TODO: In the case where we're injecting at creation time instead of run-time
// we should be skipping the extra suspends/resumes being done by the detour
// lib.

// TODO: Properly implement and test runtime injection and ejection support.

// TODO: Hook FlushInstructionCache as a lame way to detect dynamically
// allocated code (won't always be called, e.g. VAC3 doesn't use it when
// manually mapping its modules).

// TODO: Use RAII for unregistering callbacks. We have too much messy code
// duplication with potential for error (and it does happen more regularly than
// it should!).

// TODO: All detours should have reentrance guards. For example, if someone else
// is detouring WriteFile, and upon first call calls CreateFile to open its log
// file on demand, and guards against its own reentrance, and if they detour
// first, then you would be reentered.

// TODO: Support sandboxing games like SWTOR and D3.

// TODO: Anti-anti-debug plugin.

// TODO: VEH debugger.

// TODO: Add anti-stack-trace support to work around some of the more aggressive
// anti-cheats (e.g. clear stack and return to INT3, or create manual stack
// frame which matches the whitelist and single step the whole way then redirect
// control flow at the end).

// TODO: Investigate if it's possible to add specialized anti-stack-trace
// support for cases like using TLS (i.e. game sets flag in TLS before calling
// present (and resets it after), and uses it in order to detect unknown code
// calling engine functions in a Present hook).

// TODO: Add an API logger (Module32First, Process32First,
// CreateVectoredExceptionHandler, NtSetInformationThread,
// NtQuerySystemInformation, ReadProcessMemory, recv, send,
// FlushInstructionCache, etc.).

// TODO: Add generic utility classes which would be useful during both reversing
// and development. Especially those which are game or rendering related.
// E.g. Shader disassembling/dumping code. Matrix dumping code. Drawing
// primitives and text. Mathematical types like vectors and matricies, and
// helper functions like WorldToScreen.

// TODO: Add a useful generic example plugin like a Teamspeak or VLC controller.

// TODO: Add HWID-bypassing extension (e.g. Win32_BaseBoard.SerialNumber,
// Win32_BIOS.SerialNumber, Win32_BaseBoard.Manufacturer,
// Win32_BIOS.Manufacturer, Win32_OperatingSystem.InstallDate,
// Win32_OperatingSystem.SerialNumber, IOCTL_STORAGE_QUERY_PROPERTY,
// SMART_RCV_DRIVE_DATA, Windows serial, MAC address, etc.).

// TODO: Don't use so many TLS slots. Have a function which returns a pointer to
// a structure which has all the flags for disabling the hooks etc.

// TODO: Configurable hooking method. (E.g. Inline, IAT, INT3, PAGE_GUARD, etc.)

// TODO: Wrap the detour, callbacks, hook flag, etc all into one struct.

// TODO: Add scripting support. Lua? ChaiScript?

// TODO: Fix all the cases where we were previously using exceptions to control
// the order in which things occur, but now we're using callbacks which may
// swallow some exceptions.

// TODO: Ensure we're not missing locking on any global structures which may be
// accessed concurrently.

// TODO: Have two sets of OnFrame (and other?) callbacks. One for Cerberus and
// one for plugins. This will fix the unregister problem, among others.

// TODO: Fix naming of APIs. Some registration funcs are "RegisterOnFooCallback"
// while some are "RegisterOnFoo". Also, do we really need both a free and a
// member func if one just calls the other? Why the unnecessary complexity?

// TODO: To handle abstracting away GUI library perhaps have two sets of
// callbacks. A public set and an 'internal' set, that way we can control the
// order they are called. Ensure it's still possible to intentionally 'leak'
// them though, as that is sometimes desirable.

// TODO: Fix/normalize detour names. e.g. SomeFunc_Detour or
// SomeClass_SomeFunc_Detour.

// TODO: Add Pre and Post callbacks for cases like the CreateProcess hook. In
// the Pre callback hook we want to modify the args to suspend the process. In
// the Post callback hook we want to inject and then resume the process.

// TODO: Fix our dependency on undefined behaviour in regards to the order of
// static destruction of objects across translation units. Specifically, we're
// relying on this for OnUnloadPlugins in order to allow other translation units
// to get a callback early on in the static destruction 'phase', but this is
// incorrect and there is no guarantee that just because the plugin TU is
// initialized first that it will be destructed first. MSVC documents its
// behaviour in this area so it's okay to rely on it for now, but it's not a
// sustainable solution in any case.
// https://msdn.microsoft.com/en-us/library/xwec471e.aspx

// TODO: Add optional support for game 'sandboxing'. Useful for multi-boxing
// etc.

// TODO: Natively support macro keyboard/mouse buttons etc. Useful for binding
// script toggles (or other functionality) to the macro keys.

// TODO: Add builtin navigation support (separate library?).

// TODO: Add builtin AI logic framework (FSM, BT, etc.)?

// TODO: Add builtin settings (both for extensions and scripts) support.

// TODO: Add builtin logging support.

// TODO: Add builtin event support.

// TODO: Fix rendering for Trove (OpenGL, issue unknown), Marvel Heroes 2015
// (rendering to multiple viewports or something when in-game - GUI is
// double-rendered to minimap), Devilian (D3D9, no rendering or input - issue
// unknown).

// TODO: Re-architect this entire project so we're not as dependent on the
// initialization order of global statics. Perhaps use of a shared_pointer to
// manage the lifetime of components would help (assuming that components own a
// pointer to their dependencies, and we don't run into problems with cyclic
// references).

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

  hadesmem::PatchDr<void()>::InitializeStatics();

  // TODO: Move this somewhere appropriate.
  hadesmem::cerberus::GetImGuiLogWindow();
  hadesmem::cerberus::GetImguiInterface();

  // TODO: Add chaiscript.
  auto& module = hadesmem::cerberus::GetModuleInterface();
  auto& d3d9 = hadesmem::cerberus::GetD3D9Interface();
  auto& dxgi = hadesmem::cerberus::GetDXGIInterface();
  auto& render = hadesmem::cerberus::GetRenderInterface();
  auto& ant_tweak_bar = hadesmem::cerberus::GetAntTweakBarInterface();
  auto& imgui = hadesmem::cerberus::GetImguiInterface();
  auto& window = hadesmem::cerberus::GetWindowInterface();
  auto& direct_input = hadesmem::cerberus::GetDirectInputInterface();
  auto& cursor = hadesmem::cerberus::GetCursorInterface();
  auto& input = hadesmem::cerberus::GetInputInterface();
  auto& exception = hadesmem::cerberus::GetExceptionInterface();
  auto& driver = hadesmem::cerberus::GetDriverInterface();
  auto& service = hadesmem::cerberus::GetServiceInterface();
  auto& process = hadesmem::cerberus::GetProcessInterface();
  auto& raw_input = hadesmem::cerberus::GetRawInputInterface();
  auto& helper = hadesmem::cerberus::GetHelperInterface();
  (void)helper;
  auto& render_helper = hadesmem::cerberus::GetRenderHelperInterface();
  (void)render_helper;

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

  auto const on_load_callback =
    [](HMODULE, PCWSTR, PULONG, std::wstring const&, std::wstring const&)
  {
  };
  auto const on_load_id = module.RegisterOnLoad(on_load_callback);
  module.UnregisterOnLoad(on_load_id);

  auto const on_unload_callback = [](HMODULE)
  {
  };
  auto const on_unload_id = module.RegisterOnUnload(on_unload_callback);
  module.UnregisterOnUnload(on_unload_id);

  auto const on_frame_callback_d3d9 = [](IDirect3DDevice9*)
  {
  };
  auto const on_frame_id_d3d9 = d3d9.RegisterOnFrame(on_frame_callback_d3d9);
  d3d9.UnregisterOnFrame(on_frame_id_d3d9);

  auto const on_reset_callback_d3d9 = [](IDirect3DDevice9*,
                                         D3DPRESENT_PARAMETERS*)
  {
  };
  auto const on_reset_id_d3d9 = d3d9.RegisterOnReset(on_reset_callback_d3d9);
  d3d9.UnregisterOnReset(on_reset_id_d3d9);

  auto const on_release_callback_d3d9 = [](IDirect3DDevice9*)
  {
  };
  auto const on_release_id_d3d9 =
    d3d9.RegisterOnRelease(on_release_callback_d3d9);
  d3d9.UnregisterOnRelease(on_release_id_d3d9);

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

  auto const on_imgui_init = [](hadesmem::cerberus::ImguiInterface*)
  {
  };
  auto const on_imgui_init_id = imgui.RegisterOnInitialize(on_imgui_init);
  imgui.UnregisterOnInitialize(on_imgui_init_id);

  auto const on_imgui_cleanup = [](hadesmem::cerberus::ImguiInterface*)
  {
  };
  auto const on_imgui_cleanup_id = imgui.RegisterOnCleanup(on_imgui_cleanup);
  imgui.UnregisterOnCleanup(on_imgui_cleanup_id);

  auto const on_imgui_frame = []()
  {
  };
  auto const on_imgui_frame_id = imgui.RegisterOnFrame(on_imgui_frame);
  imgui.UnregisterOnFrame(on_imgui_frame_id);

  auto const on_frame = [](hadesmem::cerberus::RenderApi, void*)
  {
  };
  auto const on_frame_id = render.RegisterOnFrame(on_frame);
  render.UnregisterOnFrame(on_frame_id);

  auto const on_frame_2 = [](hadesmem::cerberus::RenderApi, void*)
  {
  };
  auto const on_frame_2_id = render.RegisterOnFrame2(on_frame_2);
  render.UnregisterOnFrame2(on_frame_2_id);

  auto const on_set_gui_visibility = [](bool, bool)
  {
  };
  auto const on_set_gui_visibility_id =
    render.RegisterOnSetGuiVisibility(on_set_gui_visibility);
  render.UnregisterOnSetGuiVisibility(on_set_gui_visibility_id);

  auto const on_initialize_gui = [](hadesmem::cerberus::RenderApi, void*)
  {
  };
  auto const on_initialize_gui_id =
    render.RegisterOnInitializeGui(on_initialize_gui);
  render.UnregisterOnInitializeGui(on_initialize_gui_id);

  auto const on_cleanup_gui = [](hadesmem::cerberus::RenderApi)
  {
  };
  auto const on_cleanup_gui_id = render.RegisterOnCleanupGui(on_cleanup_gui);
  render.UnregisterOnCleanupGui(on_cleanup_gui_id);

  auto const on_wnd_proc_msg = [](HWND, UINT, WPARAM, LPARAM, bool*)
  {
  };
  auto const on_wnd_proc_msg_id = window.RegisterOnWndProcMsg(on_wnd_proc_msg);
  window.UnregisterOnWndProcMsg(on_wnd_proc_msg_id);

  auto const on_get_foreground_window = [](bool*, HWND*)
  {
  };
  auto const on_get_foreground_window_id =
    window.RegisterOnGetForegroundWindow(on_get_foreground_window);
  window.UnregisterOnGetForegroundWindow(on_get_foreground_window_id);

  auto const on_set_cursor = [](HCURSOR, bool*, HCURSOR*)
  {
  };
  auto const on_set_cursor_id = cursor.RegisterOnSetCursor(on_set_cursor);
  cursor.UnregisterOnSetCursor(on_set_cursor_id);

  auto const on_get_cursor_pos = [](LPPOINT, bool, bool*)
  {
  };
  auto const on_get_cursor_pos_id =
    cursor.RegisterOnGetCursorPos(on_get_cursor_pos);
  cursor.UnregisterOnGetCursorPos(on_get_cursor_pos_id);

  auto const on_set_cursor_pos = [](int, int, bool, bool*)
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

  auto const on_get_device_data =
    [](DWORD, LPDIDEVICEOBJECTDATA, LPDWORD, DWORD, HRESULT*, void*, bool)
  {
  };
  auto const on_get_device_data_id =
    direct_input.RegisterOnGetDeviceData(on_get_device_data);
  direct_input.UnregisterOnGetDeviceData(on_get_device_data_id);

  auto const on_get_device_state = [](DWORD, LPVOID, HRESULT*)
  {
  };
  auto const on_get_device_state_id =
    direct_input.RegisterOnGetDeviceState(on_get_device_state);
  direct_input.UnregisterOnGetDeviceState(on_get_device_state_id);

  auto const on_input_queue_entry = [](HWND, UINT, WPARAM, LPARAM)
  {
  };
  auto const on_input_queue_entry_id =
    input.RegisterOnInputQueueEntry(on_input_queue_entry);
  input.UnregisterOnInputQueueEntry(on_input_queue_entry_id);

  auto const on_rtl_add_vectored_exception_handler =
    [](ULONG, PVECTORED_EXCEPTION_HANDLER, bool*)
  {
  };
  auto const on_rtl_add_vectored_exception_handler_id =
    exception.RegisterOnRtlAddVectoredExceptionHandler(
      on_rtl_add_vectored_exception_handler);
  exception.UnregisterOnRtlAddVectoredExceptionHandler(
    on_rtl_add_vectored_exception_handler_id);

  auto const on_set_unhandled_exception_filter =
    [](LPTOP_LEVEL_EXCEPTION_FILTER, bool*)
  {
  };
  auto const on_set_unhandled_exception_filter_id =
    exception.RegisterOnSetUnhandledExceptionFilter(
      on_set_unhandled_exception_filter);
  exception.UnregisterOnSetUnhandledExceptionFilter(
    on_set_unhandled_exception_filter_id);

  auto const on_nt_load_driver = [](PUNICODE_STRING, bool*)
  {
  };
  auto const on_nt_load_driver_id =
    driver.RegisterOnNtLoadDriver(on_nt_load_driver);
  driver.UnregisterOnNtLoadDriver(on_nt_load_driver_id);

  auto const on_create_service_a = [](SC_HANDLE,
                                      LPCSTR,
                                      LPCSTR,
                                      DWORD,
                                      DWORD,
                                      DWORD,
                                      DWORD,
                                      LPCSTR,
                                      LPCSTR,
                                      LPDWORD,
                                      LPCSTR,
                                      LPCSTR,
                                      LPCSTR,
                                      bool*)
  {
  };
  auto const on_create_service_a_id =
    service.RegisterOnCreateServiceA(on_create_service_a);
  service.UnregisterOnCreateServiceA(on_create_service_a_id);

  auto const on_create_service_w = [](SC_HANDLE,
                                      LPCWSTR,
                                      LPCWSTR,
                                      DWORD,
                                      DWORD,
                                      DWORD,
                                      DWORD,
                                      LPCWSTR,
                                      LPCWSTR,
                                      LPDWORD,
                                      LPCWSTR,
                                      LPCWSTR,
                                      LPCWSTR,
                                      bool*)
  {
  };
  auto const on_create_service_w_id =
    service.RegisterOnCreateServiceW(on_create_service_w);
  service.UnregisterOnCreateServiceW(on_create_service_w_id);

  auto const on_open_service_a = [](SC_HANDLE, LPCSTR, DWORD, bool*)
  {
  };
  auto const on_open_service_a_id =
    service.RegisterOnOpenServiceA(on_open_service_a);
  service.UnregisterOnOpenServiceA(on_open_service_a_id);

  auto const on_open_service_w = [](SC_HANDLE, LPCWSTR, DWORD, bool*)
  {
  };
  auto const on_open_service_w_id =
    service.RegisterOnOpenServiceW(on_open_service_w);
  service.UnregisterOnOpenServiceW(on_open_service_w_id);

  auto const on_create_process_internal_w = [](HANDLE,
                                               LPCWSTR,
                                               LPWSTR,
                                               LPSECURITY_ATTRIBUTES,
                                               LPSECURITY_ATTRIBUTES,
                                               BOOL,
                                               DWORD,
                                               LPVOID,
                                               LPCWSTR,
                                               LPSTARTUPINFOW,
                                               LPPROCESS_INFORMATION,
                                               PHANDLE,
                                               bool*,
                                               BOOL*,
                                               bool*)
  {
  };
  auto const on_create_process_internal_w_id =
    process.RegisterOnCreateProcessInternalW(on_create_process_internal_w);
  process.UnregisterOnCreateProcessInternalW(on_create_process_internal_w_id);

  auto const on_rtl_exit_user_process = [](NTSTATUS)
  {
  };
  auto const on_rtl_exit_user_process_id =
    process.RegisterOnRtlExitUserProcess(on_rtl_exit_user_process);
  process.UnregisterOnRtlExitUserProcess(on_rtl_exit_user_process_id);

  auto const on_get_raw_input_buffer = [](PRAWINPUT, PUINT, UINT, bool*, UINT*)
  {
  };
  auto const on_get_raw_input_buffer_id =
    raw_input.RegisterOnGetRawInputBuffer(on_get_raw_input_buffer);
  raw_input.UnregisterOnGetRawInputBuffer(on_get_raw_input_buffer_id);

  auto const on_get_raw_input_data =
    [](HRAWINPUT, UINT, LPVOID, PUINT, UINT, bool*, UINT*)
  {
  };
  auto const on_get_raw_input_data_id =
    raw_input.RegisterOnGetRawInputData(on_get_raw_input_data);
  raw_input.UnregisterOnGetRawInputData(on_get_raw_input_data_id);

  auto const on_register_raw_input_devices =
    [](PCRAWINPUTDEVICE, UINT, UINT, bool*, BOOL*)
  {
  };
  auto const on_register_raw_input_devices_id =
    raw_input.RegisterOnRegisterRawInputDevices(on_register_raw_input_devices);
  raw_input.UnregisterOnRegisterRawInputDevices(
    on_register_raw_input_devices_id);

  // Must be before plugins, because they attempt to reset scripts on unload.
  // TODO: Move this somewhere appropriate.
  auto& chai = hadesmem::cerberus::GetChaiScriptInterface();
  chai.GetGlobalContext();
  auto const on_init_chai_id = chai.RegisterOnInitializeChaiScriptContext(
    [](chaiscript::ChaiScript& /*chai*/)
    {
    });
  chai.UnregisterOnInitializeChaiScriptContext(on_init_chai_id);
  hadesmem::cerberus::GetChaiScriptScripts();
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
  auto const this_module_size = hadesmem::detail::GetModuleRegionSize(
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

extern "C" __declspec(dllexport) DWORD_PTR Load() noexcept
{
  try
  {
    std::mutex& mutex = GetInitializeMutex();
    std::lock_guard<std::mutex> lock{mutex};

    bool& is_initialized = GetInititalizeFlag();
    if (is_initialized)
    {
      HADESMEM_DETAIL_TRACE_A("Already initialized. Bailing.");
      return 2;
    }

    is_initialized = true;

    // Suspend process before doing anything for safety reasons when doing
    // runtime injection (less important when done at process creation as
    // there's only one thread and it's already suspended, and new threads are
    // unlikely to be created/injected except for ours).
    // TODO: Identify the problems that were solved by this hack and fix them
    // properly. We can't do this because another thread might currently hold a
    // lock we need (loader lock, heap lock, etc.).
    // Alternatively, we could acquire the locks ourselves before suspending
    // other threads, but that's not really viable/possible in all scenarios...
    // auto const& process = hadesmem::cerberus::GetThisProcess();
    // hadesmem::SuspendedProcess suspend{process.GetId()};

    auto const& config = hadesmem::cerberus::GetConfig();

    UseAllStatics();

    hadesmem::cerberus::LoadPlugins();

    if (config.IsAntTweakBarEnabled())
    {
      HADESMEM_DETAIL_TRACE_A("Initializing AntTweakBar support.");
      hadesmem::cerberus::InitializeAntTweakBar();
    }

    if (config.IsImguiEnabled())
    {
      HADESMEM_DETAIL_TRACE_A("Initializing Imgui support.");
      hadesmem::cerberus::InitializeImgui();
    }

    // The order of some of these calls is important. E.g. The GUI libs
    // are initialized before the renderer, and the renderer is initialized
    // before the actual rendering API hooks. This is to avoid race conditions
    // that would otherwise be caused if we initialized the renderer before the
    // GUI lib, which means the initialization callback would never be called
    // and then we would attempt to draw when in an uninitialized state.
    // We also load plugins first so they can register all their callbacks
    // because otherwise things like GUI visibility callbacks might be missed.
    // Obviously all of this only applies for injection at run-time rather than
    // launch-time, so it's not a big deal yet because support for that is not
    // complete anyway.
    // TODO: Solve this by suspending the process in this function instead of
    // relying on the constant suspend/resumes in the patcher? The
    // suspend/resumes in the patcher works fine for hooking safety, but simply
    // suspending in this function would allow us to avoid all the race
    // conditions and initialize everything in whatever order we want.
    // We still need the auto-suspension in the patcher even if we suspend here
    // though, because the same hooking code is used during OnModuleLoad etc
    // events where the process is not frozen. We could work around that by
    // manually adding suspend code everywhere that is appropriate... Needs more
    // investigation though.
    hadesmem::cerberus::InitializeRenderHelper();
    hadesmem::cerberus::InitializeModule();
    hadesmem::cerberus::InitializeException();
    hadesmem::cerberus::InitializeDriver();
    hadesmem::cerberus::InitializeService();
    hadesmem::cerberus::InitializeProcess();
    hadesmem::cerberus::InitializeRender();
    hadesmem::cerberus::InitializeD3D9();
    hadesmem::cerberus::InitializeDXGI();
    hadesmem::cerberus::InitializeOpenGL32();
    hadesmem::cerberus::InitializeInput();
    hadesmem::cerberus::InitializeWindow();
    hadesmem::cerberus::InitializeDirectInput();
    hadesmem::cerberus::InitializeCursor();
    hadesmem::cerberus::InitializeRawInput();

    // TODO: Move this to the initialization functions?
    hadesmem::cerberus::DetourNtdllForModule(nullptr);
    hadesmem::cerberus::DetourNtdllForException(nullptr);
    hadesmem::cerberus::DetourKernelBaseForException(nullptr);
    hadesmem::cerberus::DetourNtdllForDriver(nullptr);
    hadesmem::cerberus::DetourSechostForService(nullptr);
    hadesmem::cerberus::DetourKernelBaseForProcess(nullptr);
    hadesmem::cerberus::DetourNtdllForProcess(nullptr);
    hadesmem::cerberus::DetourD3D9(nullptr);
    hadesmem::cerberus::DetourDXGI(nullptr);
    hadesmem::cerberus::DetourDirectInput8(nullptr);
    hadesmem::cerberus::DetourUser32ForCursor(nullptr);
    hadesmem::cerberus::DetourUser32ForRawInput(nullptr);
    hadesmem::cerberus::DetourUser32ForWindow(nullptr);
    hadesmem::cerberus::DetourOpenGL32(nullptr);

    HADESMEM_DETAIL_TRACE_A("Finished initialization.");

    return 0;
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    // Don't assert here because if we do then the process will start to
    // terminate, and plugins will have their DllMain called for unload before
    // we have a chance to clean them up properly. This will result in crashes
    // and other nastiness due to use-after-free, double-free, etc.
    // TODO: Confirm this is actually what was happening.
    // HADESMEM_DETAIL_ASSERT(false);

    return 1;
  }
}

// TODO: Properly handle the case where we are free'd while the GUI is visible.
// Right now we don't do things like restoring cursor state etc.
extern "C" __declspec(dllexport) DWORD_PTR Free() noexcept
{
  try
  {
    std::mutex& mutex = GetInitializeMutex();
    std::lock_guard<std::mutex> lock{mutex};

    bool& is_initialized = GetInititalizeFlag();
    if (!is_initialized)
    {
      HADESMEM_DETAIL_TRACE_A("Already cleaned up. Bailing.");
      return 0;
    }

    is_initialized = false;

    HADESMEM_DETAIL_TRACE_A("Removing detours.");

    hadesmem::cerberus::UndetourNtdllForModule(true);
    hadesmem::cerberus::UndetourNtdllForException(true);
    hadesmem::cerberus::UndetourKernelBaseForException(true);
    hadesmem::cerberus::UndetourNtdllForDriver(true);
    hadesmem::cerberus::UndetourSechostForService(true);
    hadesmem::cerberus::UndetourKernelBaseForProcess(true);
    hadesmem::cerberus::UndetourNtdllForProcess(true);
    hadesmem::cerberus::UndetourDXGI(true);
    hadesmem::cerberus::UndetourD3D9(true);
    hadesmem::cerberus::UndetourDirectInput8(true);
    hadesmem::cerberus::UndetourUser32ForCursor(true);
    hadesmem::cerberus::UndetourUser32ForRawInput(true);
    hadesmem::cerberus::UndetourUser32ForWindow(true);
    hadesmem::cerberus::UndetourOpenGL32(true);

    HADESMEM_DETAIL_TRACE_A("Unloading plugins.");

    hadesmem::cerberus::UnloadPlugins();

    HADESMEM_DETAIL_TRACE_A("Cleaning up.");

    hadesmem::cerberus::CleanupRender();

    HADESMEM_DETAIL_TRACE_A("Doing saninty checks.");

    // TODO: Actually check this return value in the injector and don't call
    // FreeLibrary?
    if (!IsSafeToUnload())
    {
      return 2;
    }

    HADESMEM_DETAIL_TRACE_A("Done.");

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
                    LPVOID /*reserved*/) noexcept
{
  return TRUE;
}

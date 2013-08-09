// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "d3d9_manager.hpp"

#include <array>
#include <mutex>
#include <memory>
#include <iostream>
#include <algorithm>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/thread.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <windows.h>
#include <winternl.h>
#include <winnt.h>

// Work around a Clang issue.
// c:/mingw32-dw2/bin/../lib/clang/3.2/../../../i686-w64-mingw32/include
// \d3dx9math.inl:994:15: error: use of undeclared identifier 'max'; did 
// you mean 'fmax'?
// TODO: Figure out whether this is the fault of Clang, or MinGW, or 
// something else.
#include <hadesmem/config.hpp>
#if defined(HADESMEM_CLANG)
using std::min;
using std::max;
#endif // #if defined(HADESMEM_CLANG)

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <d3d9.h>
#include <d3dx9.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/injector.hpp>
#include <hadesmem/detail/self_path.hpp>
#include <hadesmem/detail/smart_handle.hpp>
#include <hadesmem/detail/static_assert.hpp>
#include <hadesmem/detail/to_upper_ordinal.hpp>

#include "d3d9_device.hpp"

// TODO: Clean this all up.

// TODO: Add more error checking.

// TODO: Thread safety and (if appliciable) reentrancy safety checks.

// TODO: Hook process creation APIs at NT level (NtCreateUserProcess, 
// NtCreateProcess, NtCreateProcessEx).

// TODO: Investigate how ShellExecute[Ex], WinExec, etc create processes and 
// how we should hook them.

// TODO: Support 32-bit processes running 64-bit processes and vice-versa.

// TODO: Support multiple D3D modules (both in parallel and reloading of a 
// single module).

// TODO: Fail gracefully if any functions we hook don't exist.

// TODO: Should the D3D hooks be destroyed automatically instead of by a 
// cleanup function? What if d3d9hook.dll is unloaded before d3d9 and 
// something calls one of the funcs we hook? Probably unlikely.. The 
// hooks that are likely to be called (e.g. LdrLoadDll) are already 
// automatically cleaned up. We do need to make sure we don't unload at a 
// time that will cause the target to crash though... Investigate safety 
// for unloads at arbitrary times (and perhaps reloads using a save and 
// lookup of some sort, to make testing easier).

// TODO: Improve module load hooking/detection. Currently if D3D9.dll is 
// loaded in response to the static imports of a module (but not the process 
// proper) then we will miss it! Example of a game doing this is Left 4 Dead.

// TODO: Make it safe to unload on the fly.

// TODO: Find the right headers so that this can be removed.
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS (static_cast<NTSTATUS>(0x00000000L))
#endif
#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) ((static_cast<NTSTATUS>(Status)) >= 0)
#endif

// TODO: Fix the code so this hack isn't required.
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif // #if defined(HADESMEM_CLANG)

namespace
{

std::unique_ptr<hadesmem::Process> g_process;

typedef std::unique_ptr<hadesmem::detail::SmartModuleHandle> 
  SmartModuleHandlePtr;

typedef std::unique_ptr<hadesmem::PatchDetour> PatchDetourPtr;

struct ModuleData
{

PatchDetourPtr d3d9_create_hk;
PatchDetourPtr d3d9_create_ex_hk;
// TODO: Replace this with a proxy interface.
PatchDetourPtr create_device_hk;

};

std::pair<std::unique_ptr<hadesmem::detail::SmartModuleHandle>, 
  std::unique_ptr<ModuleData>> g_d3d_mod;

PatchDetourPtr g_ldr_load_dll;

PatchDetourPtr g_create_process_internal_w;

// TODO: Fix this hack.
auto const to_string = 
  [] (unsigned long) -> std::string
  {
    std::stringstream str;
    str.imbue(std::locale::classic());
    return str.str();
  };

IDirect3D9* WINAPI Direct3DCreate9Hk(UINT sdk_version);

HRESULT WINAPI Direct3DCreate9ExHk(UINT sdk_version, IDirect3D9Ex** ppd3d9);

HRESULT WINAPI CreateDeviceHk(IDirect3D9* pd3d9, 
  UINT adapter, 
  D3DDEVTYPE device_type, 
  HWND focus_wnd, 
  DWORD behaviour_flags, 
  D3DPRESENT_PARAMETERS* presentation_params, 
  IDirect3DDevice9** ppdevice);

void ApplyDetours(hadesmem::Module const& d3d9_mod);

NTSTATUS WINAPI LdrLoadDllHk(PCWSTR path, PULONG characteristics, 
  PCUNICODE_STRING name,  PVOID* handle);

BOOL WINAPI CreateProcessInternalWHk(
  HANDLE hUserToken,  
  LPCWSTR lpApplicationName,  
  LPWSTR lpCommandLine,  
  LPSECURITY_ATTRIBUTES lpProcessAttributes,  
  LPSECURITY_ATTRIBUTES lpThreadAttributes,  
  BOOL bInheritHandles,  
  DWORD dwCreationFlags,  
  LPVOID lpEnvironment,  
  LPCWSTR lpCurrentDirectory,  
  LPSTARTUPINFOW lpStartupInfo,  
  LPPROCESS_INFORMATION lpProcessInformation,  
  PHANDLE hRestrictedUserToken);

IDirect3D9* WINAPI Direct3DCreate9Hk(UINT sdk_version)
{
  IDirect3D9* d3d9 = nullptr;
  DWORD last_error = 0xDEADBEEF;
  try
  {
    static boost::mutex mutex;
    boost::lock_guard<boost::mutex> lock(mutex);

    HADESMEM_TRACE_A("Direct3DCreate9Hk called.\n");
  
    auto const d3d9_create = 
      reinterpret_cast<decltype(&Direct3DCreate9Hk)>(
      reinterpret_cast<DWORD_PTR>(
      g_d3d_mod.second->d3d9_create_hk->GetTrampoline()));

    d3d9 = d3d9_create(sdk_version);
    last_error = ::GetLastError();
    if (d3d9)
    {
      HADESMEM_TRACE_A("Direct3DCreate9 succeeded.\n");

      if (!g_d3d_mod.second->create_device_hk)
      {
        PBYTE* vmt = *reinterpret_cast<PBYTE**>(d3d9);

        PBYTE create_device = vmt[16];
        PBYTE create_device_hk = reinterpret_cast<PBYTE>(
          reinterpret_cast<DWORD_PTR>(&CreateDeviceHk));

        g_d3d_mod.second->create_device_hk.reset(new hadesmem::PatchDetour(
          *g_process, create_device, create_device_hk));
        g_d3d_mod.second->create_device_hk->Apply();
      }
    }
    else
    {
      HADESMEM_TRACE_A("Direct3DCreate9 failed.\n");
    }
  }
  catch (std::exception const& e)
  {
    HADESMEM_TRACE_A((boost::diagnostic_information(e) + "\n").c_str());
  }

  ::SetLastError(last_error);
  return d3d9;
}

HRESULT WINAPI Direct3DCreate9ExHk(UINT sdk_version, IDirect3D9Ex** ppd3d9)
{

  HRESULT hr = ERROR_NOT_SUPPORTED;
  DWORD last_error = 0;
  
  try
  {
    static boost::mutex mutex;
    boost::lock_guard<boost::mutex> lock(mutex);

    HADESMEM_TRACE_A("Direct3DCreate9ExHk called.\n");

    auto const d3d9_create_ex = 
      reinterpret_cast<decltype(&Direct3DCreate9ExHk)>(
      reinterpret_cast<DWORD_PTR>(
      g_d3d_mod.second->d3d9_create_ex_hk->GetTrampoline()));

    hr = d3d9_create_ex(sdk_version, ppd3d9);
    last_error = ::GetLastError();
    if (SUCCEEDED(hr) && ppd3d9 && *ppd3d9)
    {
      HADESMEM_TRACE_A("Direct3DCreate9Ex succeeded.\n");

      if (!g_d3d_mod.second->create_device_hk)
      {
        PBYTE* vmt = *reinterpret_cast<PBYTE**>(*ppd3d9);

        PBYTE create_device = vmt[16];
        PBYTE create_device_hk = reinterpret_cast<PBYTE>(
          reinterpret_cast<DWORD_PTR>(&CreateDeviceHk));

        g_d3d_mod.second->create_device_hk.reset(new hadesmem::PatchDetour(
          *g_process, create_device, create_device_hk));
        g_d3d_mod.second->create_device_hk->Apply();
      }
    }
    else
    {
      HADESMEM_TRACE_A("Direct3DCreate9Ex failed.\n");
    }
  }
  catch (std::exception const& e)
  {
    HADESMEM_TRACE_A((boost::diagnostic_information(e) + "\n").c_str());
  }

  ::SetLastError(last_error);
  return hr;
}

HRESULT WINAPI CreateDeviceHk(IDirect3D9* pd3d9, 
  UINT adapter, 
  D3DDEVTYPE device_type, 
  HWND focus_wnd, 
  DWORD behaviour_flags, 
  D3DPRESENT_PARAMETERS* presentation_params, 
  IDirect3DDevice9** ppdevice)
{
  HRESULT hr = ERROR_NOT_SUPPORTED;
  DWORD last_error_restored = 0;
  
  try
  {
    static boost::mutex mutex;
    boost::lock_guard<boost::mutex> lock(mutex);
    
    HADESMEM_TRACE_A("CreateDeviceHk called.\n");

    auto const create_device = 
      reinterpret_cast<decltype(&CreateDeviceHk)>(
      reinterpret_cast<DWORD_PTR>(
      g_d3d_mod.second->create_device_hk->GetTrampoline()));

    hr = create_device(pd3d9, adapter, device_type, focus_wnd, 
      behaviour_flags, presentation_params, ppdevice);
    last_error_restored = ::GetLastError();
    if (SUCCEEDED(hr) && ppdevice && *ppdevice)
    {
      HADESMEM_TRACE_A("CreateDevice succeeded.\n");
      
      IDirect3DDevice9Proxy* proxy = new IDirect3DDevice9Proxy(*ppdevice);
      *ppdevice = proxy;
      (void)proxy;

      // TODO: Add code to clean up the proxy eventually. This would have to 
      // be done very carefully. The proxy can only safely be unloaded once 
      // the associated d3d9.dll has been unloaded.
    }
    else
    {
      HADESMEM_TRACE_A("CreateDevice failed.\n");
    }
  }
  catch (std::exception const& e)
  {
    HADESMEM_TRACE_A((boost::diagnostic_information(e) + "\n").c_str());
  }

  ::SetLastError(last_error_restored);
  return hr;
}

// TODO: Fix this to properly support module unloads and loads, 
// then to support multiple modules simultaneously.
void ApplyDetours(hadesmem::Module const& d3d9_mod)
{
  static boost::mutex mutex;
  boost::lock_guard<boost::mutex> lock(mutex);
  
  if (g_d3d_mod.first != nullptr && g_d3d_mod.first->GetHandle() != d3d9_mod.GetHandle())
  {
    HADESMEM_TRACE_A("Attempt to apply detours to a new module when the "
      "initial module is still loaded.\n");
    return;
  }

  if (g_d3d_mod.first != nullptr)
  {
    HADESMEM_TRACE_A("Reload of same D3D9 module. Ignoring.\n");
    return;
  }

  if (g_d3d_mod.first == nullptr)
  {
    HADESMEM_TRACE_A("New D3D9 module. Hooking.\n");

    HADESMEM_ASSERT(!g_d3d_mod.second);

    std::pair<std::unique_ptr<hadesmem::detail::SmartModuleHandle>, 
      std::unique_ptr<ModuleData>> d3d9_mod_tmp;
    d3d9_mod_tmp.first.reset(new hadesmem::detail::SmartModuleHandle());
    d3d9_mod_tmp.second.reset(new ModuleData());
    HMODULE d3d9_handle_tmp = nullptr;
    if (!::GetModuleHandleEx(
      GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, 
      reinterpret_cast<LPCTSTR>(d3d9_mod.GetHandle()), 
      &d3d9_handle_tmp))
    {
      HADESMEM_TRACE_A("Attempt to bump ref count of D3D9 module "
        "failed.\n");
      return;
    }
    *d3d9_mod_tmp.first = d3d9_handle_tmp;
    
    FARPROC const d3d9_create = hadesmem::FindProcedure(*g_process, 
      d3d9_mod, "Direct3DCreate9");
    if (d3d9_create)
    {
      PVOID target = reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
        d3d9_create));
      PVOID detour = reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
        &Direct3DCreate9Hk));
      d3d9_mod_tmp.second->d3d9_create_hk.reset(
        new hadesmem::PatchDetour(
        *g_process, target, detour));
      d3d9_mod_tmp.second->d3d9_create_hk->Apply();
    }
    else
    {
      HADESMEM_TRACE_A("Failed to find d3d9.dll!Direct3DCreate9.");
    }
    
    FARPROC const d3d9_create_ex = hadesmem::FindProcedure(*g_process, 
      d3d9_mod, "Direct3DCreate9Ex");
    if (d3d9_create_ex)
    {
      PVOID target = reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
        d3d9_create_ex));
      PVOID detour = reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
        &Direct3DCreate9ExHk));
      d3d9_mod_tmp.second->d3d9_create_ex_hk.reset(
        new hadesmem::PatchDetour(
        *g_process, target, detour));
      d3d9_mod_tmp.second->d3d9_create_ex_hk->Apply();
    }
    else
    {
      HADESMEM_TRACE_A("Failed to find d3d9.dll!Direct3DCreate9Ex.");
    }

    g_d3d_mod.first = std::move(d3d9_mod_tmp.first);
    g_d3d_mod.second = std::move(d3d9_mod_tmp.second);
  }
}

NTSTATUS WINAPI LdrLoadDllHk(PCWSTR path, PULONG characteristics, 
  PCUNICODE_STRING name,  PVOID* handle)
{
  // Nothing must load a DLL until the the in_hook flag is set to true.

  static boost::thread_specific_ptr<bool> in_hook;
  if (in_hook.get() == nullptr)
  {
    in_hook.reset(new bool(false));
  }
  
  NTSTATUS ret = STATUS_SUCCESS;
  DWORD last_error = 0;
  
  auto const ldr_load_dll = 
    reinterpret_cast<decltype(&LdrLoadDllHk)>(
    reinterpret_cast<DWORD_PTR>(
    g_ldr_load_dll->GetTrampoline()));
  
  if (*in_hook == true)
  {
    return ldr_load_dll(path, characteristics, name, handle);
  }

  *in_hook = true;

  HADESMEM_TRACE_A("LdrLoadDll called.\n");

  try
  {
    ret = ldr_load_dll(path, characteristics, name, handle);
    last_error = GetLastError();
    if (NT_SUCCESS(ret))
    {
      HADESMEM_TRACE_A("LdrLoadDll succeeded.\n");
      
      if (name && name->Length && name->Buffer)
      {
        // TODO: Optimize this.
        // TODO: Should be checking anything else here like characteristics, 
        // path, handle, etc?

        auto const null_char = std::find(name->Buffer, 
          name->Buffer + name->Length, L'\0');
        std::wstring const name_real(hadesmem::detail::ToUpperOrdinal(
          std::wstring(name->Buffer, null_char)));
        HADESMEM_TRACE_W((L"Loaded DLL: " + name_real + L".\n").c_str());
        std::wstring const d3d9_dll_name(hadesmem::detail::ToUpperOrdinal(
          L"d3d9.dll"));
        std::wstring const d3d9_name(hadesmem::detail::ToUpperOrdinal(
          L"d3d9"));
        if (name_real == d3d9_dll_name || name_real == d3d9_name)
        {
          HADESMEM_TRACE_A("D3D9 loaded. Applying hooks.\n");
          
          hadesmem::Module const d3d9_mod(*g_process, 
            reinterpret_cast<HMODULE>(*handle));
          ApplyDetours(d3d9_mod);
        }
      }
    }
    else
    {
      HADESMEM_TRACE_A("LdrLoadDll failed.\n");
    }
  }
  catch (std::exception const& e)
  {
    HADESMEM_TRACE_A((boost::diagnostic_information(e) + "\n").c_str());
  }

  *in_hook = false;

  ::SetLastError(last_error);
  return ret;
}

BOOL WINAPI CreateProcessInternalWHk(
  HANDLE user_token,  
  LPCWSTR application_name,  
  LPWSTR command_line,  
  LPSECURITY_ATTRIBUTES process_attributes,  
  LPSECURITY_ATTRIBUTES thread_attributes,  
  BOOL inherit_handles,  
  DWORD creation_flags,  
  LPVOID environment,  
  LPCWSTR current_directory,  
  LPSTARTUPINFOW startup_info,  
  LPPROCESS_INFORMATION process_info,  
  PHANDLE restricted_user_token)
{
  HADESMEM_TRACE_A("CreateProcessInternalW called.\n");

  BOOL ret = TRUE;
  DWORD last_error = 0;
  
  try
  {
    auto const create_process_internal_w = 
      reinterpret_cast<decltype(&CreateProcessInternalWHk)>(
      reinterpret_cast<DWORD_PTR>(
      g_create_process_internal_w->GetTrampoline()));

    DWORD const creation_flags_new = creation_flags | CREATE_SUSPENDED;
    ret = create_process_internal_w(
      user_token,  
      application_name,  
      command_line,  
      process_attributes,  
      thread_attributes,  
      inherit_handles,  
      creation_flags_new,  
      environment,  
      current_directory,  
      startup_info,  
      process_info,  
      restricted_user_token);
    last_error = GetLastError();
    if (NT_SUCCESS(ret))
    {
      HADESMEM_TRACE_A("CreateProcessInternalW succeeded.\n");

      DWORD const pid = process_info->dwProcessId;

#if !defined(HADESMEM_NO_TRACING)
      std::stringstream pid_str;
      pid_str.imbue(std::locale::classic());
      pid_str << "PID: " << pid << ".\n";
#endif
      HADESMEM_TRACE_A(pid_str.str().c_str());

      HADESMEM_TRACE_A("Opening process.\n");
      hadesmem::Process const process(pid);
      HADESMEM_TRACE_A("Getting path to self.\n");
      std::wstring const self_path = hadesmem::detail::GetSelfPath();
      HADESMEM_TRACE_W((L"Path: \"" + self_path + L"\".\n").c_str());
      HADESMEM_TRACE_A("Injecting DLL.\n");
      HMODULE const remote_mod = hadesmem::InjectDll(process, self_path, 
        hadesmem::InjectFlags::kAddToSearchOrder);
      // TODO: Configurable export name
      HADESMEM_TRACE_A("Calling Load export.\n");
      hadesmem::CallResult<DWORD_PTR> init_result = hadesmem::CallExport(
        process, remote_mod, "Load");

#if !defined(HADESMEM_NO_TRACING)
      std::stringstream export_result;
      export_result.imbue(std::locale::classic());
      export_result 
        << "Export result: " 
        << std::hex 
        << "0x" 
        << init_result.GetReturnValue() 
        << " 0x" 
        << init_result.GetLastError() 
        << ".\n";
#endif
      HADESMEM_TRACE_A(export_result.str().c_str());
    }
    else
    {
      HADESMEM_TRACE_A("CreateProcessInternalW failed.\n");
    }
  }
  catch (std::exception const& e)
  {
    HADESMEM_TRACE_A((boost::diagnostic_information(e) + "\n").c_str());
  }

  try
  {
    if (ret && (creation_flags & CREATE_SUSPENDED) == 0)
    {
      if (::ResumeThread(process_info->hThread) == static_cast<DWORD>(-1))
      {
        DWORD const last_error_inner = ::GetLastError();
        HADESMEM_THROW_EXCEPTION(hadesmem::Error() << 
          hadesmem::ErrorString("ResumeThread failed.") << 
          hadesmem::ErrorCodeWinLast(last_error_inner));
      }
    }
  }
  catch (std::exception const& e)
  {
    HADESMEM_TRACE_A((boost::diagnostic_information(e) + "\n").c_str());

    ::TerminateProcess(process_info->hProcess, 0xDEADBEEF);
  }
  
  ::SetLastError(last_error);
  return ret;
}

}

void InitializeD3D9Hooks()
{
  static boost::mutex mutex;
  boost::lock_guard<boost::mutex> lock(mutex);
  
  g_process.reset(new hadesmem::Process(::GetCurrentProcessId()));
  
  {
    HADESMEM_TRACE_A("Hooking CreateProcessInternalW.\n");

    // TODO: Support multiple CreateProcessInternalW hooks.
    // TODO: Fall back gracefully if this API doesn't exist.
    // TODO: Fall back to kernel32 if we can't find kernelbase.
    hadesmem::Module kernelbase(*g_process, L"kernelbase.dll");
    FARPROC const create_process_internal_w = hadesmem::FindProcedure(
      *g_process, kernelbase, "CreateProcessInternalW");
    PVOID target = reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
      create_process_internal_w));
    PVOID detour = reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
      &CreateProcessInternalWHk));
    g_create_process_internal_w.reset(new hadesmem::PatchDetour(
      *g_process, target, detour));
    g_create_process_internal_w->Apply();
  }
  
  std::unique_ptr<hadesmem::Module> d3d9_mod;
  try
  {
    d3d9_mod.reset(new hadesmem::Module(*g_process, L"d3d9.dll"));
  }
  catch (std::exception const& /*e*/)
  {
    HADESMEM_TRACE_A("Hooking LdrLoadDll.\n");

    // TODO: Support multiple LdrLoadDll hooks.
    // TODO: Fall back gracefully if this API doesn't exist.
    hadesmem::Module ntdll(*g_process, L"ntdll.dll");
    FARPROC const ldr_load_dll = hadesmem::FindProcedure(*g_process, ntdll, 
      "LdrLoadDll");
    PVOID target = reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
      ldr_load_dll));
    PVOID detour = reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
      &LdrLoadDllHk));
    g_ldr_load_dll.reset(new hadesmem::PatchDetour(*g_process, target, 
      detour));
    g_ldr_load_dll->Apply();

    return;
  }

  HADESMEM_TRACE_A("Hooking D3D9 directly.\n");

  ApplyDetours(*d3d9_mod);
}

void UninitializeD3D9Hooks()
{
  static boost::mutex mutex;
  boost::lock_guard<boost::mutex> lock(mutex);
  
  if (g_d3d_mod.first)
  {
    HADESMEM_TRACE_A("Cleaning up D3D mod handle.\n");
    g_d3d_mod.first = nullptr;
  }
  
  if (g_d3d_mod.second)
  {
    HADESMEM_TRACE_A("Cleaning up D3D mod data.\n");
    g_d3d_mod.second = nullptr;
  }
}

// TODO: Fix the code so this hack isn't required.
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic pop
#endif // #if defined(HADESMEM_CLANG)

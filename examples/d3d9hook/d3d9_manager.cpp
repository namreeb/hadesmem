// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "d3d9_manager.hpp"

#include <array>
#include <mutex>
#include <memory>
#include <string>
#include <cstddef>
#include <iostream>
#include <algorithm>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/thread.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <windows.h>
#include <winternl.h>
#include <winnt.h>
#include <intrin.h>

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
#include <hadesmem/region.hpp>
#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/injector.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/detail/self_path.hpp>
#include <hadesmem/detail/make_unique.hpp>
#include <hadesmem/detail/smart_handle.hpp>
#include <hadesmem/detail/static_assert.hpp>
#include <hadesmem/detail/find_procedure.hpp>
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

// TODO: Investigate places where D3D9Hook should be bumping the DLL 
// reference count (using GetModuleHandleEx).

// TODO: Probe memory in API hooks safely.

// TODO: Use RAII to ensure the last error code is restored.

// TODO: Find the right headers so that this can be removed.
#if !defined(NT_SUCCESS)
#define NT_SUCCESS(Status) ((static_cast<NTSTATUS>(Status)) >= 0)
#endif

// TODO: Clean this up, and move to its own header or find the right 
// existing headers.
// TODO: Stuff is currently named wrong to fix name collisions. Fix this 
// properly.
extern "C"
{

enum HADESMEM_SECTION_INHERIT
{
  ViewShare = 1, 
  ViewUnmap = 2 
};

struct HADESMEM_TIB
{
  struct _EXCEPTION_REGISTRATION_RECORD* ExceptionList;
  PVOID StackBase;
  PVOID StackLimit;
  PVOID SubSystemTib;
  PVOID FiberData;
  PVOID ArbitraryUserPointer;
  struct HADESMEM_TIB *Self;
};

struct HADESMEM_TEB
{
  HADESMEM_TIB Tib;
  // More stuff here that we currently don't care about.
};

#if defined(HADESMEM_DETAIL_ARCH_X64) 
inline HADESMEM_TEB* HadesMemCurrentTeb()
{
  return reinterpret_cast<HADESMEM_TEB*>(
    __readgsqword(offsetof(NT_TIB, Self)));
}
#elif defined(HADESMEM_DETAIL_ARCH_X86) 
inline HADESMEM_TEB* HadesMemCurrentTeb()
{
  return reinterpret_cast<HADESMEM_TEB*>(
    __readfsdword(offsetof(NT_TIB, Self)));
}
#else 
#error "[HadesMem] Unsupported architecture."
#endif

}

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

PatchDetourPtr g_nt_map_view_of_section;

PatchDetourPtr g_create_process_internal_w;

// TODO: Fix this hack.
auto const to_string = 
  [] (unsigned long) -> std::string
  {
    std::stringstream str;
    str.imbue(std::locale::classic());
    return str.str();
  };

extern "C" IDirect3D9* WINAPI Direct3DCreate9Hk(UINT sdk_version);

extern "C" HRESULT WINAPI Direct3DCreate9ExHk(UINT sdk_version, IDirect3D9Ex** ppd3d9);

extern "C" HRESULT WINAPI CreateDeviceHk(IDirect3D9* pd3d9, 
  UINT adapter, 
  D3DDEVTYPE device_type, 
  HWND focus_wnd, 
  DWORD behaviour_flags, 
  D3DPRESENT_PARAMETERS* presentation_params, 
  IDirect3DDevice9** ppdevice);

void ApplyDetours(HMODULE d3d9_mod);

extern "C" NTSTATUS WINAPI NtMapViewOfSectionHk(
  HANDLE section, 
  HANDLE process, 
  PVOID* base, 
  ULONG_PTR zero_bits, 
  SIZE_T commit_size, 
  PLARGE_INTEGER section_offset, 
  PSIZE_T view_size, 
  HADESMEM_SECTION_INHERIT inherit_disposition, 
  ULONG alloc_type, 
  ULONG alloc_protect);

extern "C" BOOL WINAPI CreateProcessInternalWHk(
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

extern "C" IDirect3D9* WINAPI Direct3DCreate9Hk(UINT sdk_version)
{
  IDirect3D9* d3d9 = reinterpret_cast<IDirect3D9*>(0xDEADBEEF);
  DWORD last_error = 0xDEADBEEF;
  try
  {
    static boost::mutex mutex;
    boost::lock_guard<boost::mutex> lock(mutex);

    HADESMEM_DETAIL_TRACE_A("Direct3DCreate9Hk called.");
  
    auto const d3d9_create = g_d3d_mod.second->d3d9_create_hk->
      GetTrampoline<decltype(&Direct3DCreate9Hk)>();

    d3d9 = d3d9_create(sdk_version);
    last_error = ::GetLastError();
    if (d3d9)
    {
      HADESMEM_DETAIL_TRACE_A("Direct3DCreate9 succeeded.");

      if (!g_d3d_mod.second->create_device_hk)
      {
        PBYTE* vmt = *reinterpret_cast<PBYTE**>(d3d9);

        PBYTE create_device = vmt[16];
        PBYTE create_device_hk = reinterpret_cast<PBYTE>(
          reinterpret_cast<DWORD_PTR>(&CreateDeviceHk));

        g_d3d_mod.second->create_device_hk = 
          hadesmem::detail::make_unique<hadesmem::PatchDetour>(
          *g_process, create_device, create_device_hk);
        g_d3d_mod.second->create_device_hk->Apply();
      }
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("Direct3DCreate9 failed.");
    }
  }
  catch (std::exception const& e)
  {
    HADESMEM_DETAIL_TRACE_A(boost::diagnostic_information(e).c_str());
  }

  ::SetLastError(last_error);
  return d3d9;
}

extern "C" HRESULT WINAPI Direct3DCreate9ExHk(UINT sdk_version, IDirect3D9Ex** ppd3d9)
{

  HRESULT hr = static_cast<HRESULT>(0xDEADBEEF);
  DWORD last_error = 0xDEADBEEF;
  
  try
  {
    static boost::mutex mutex;
    boost::lock_guard<boost::mutex> lock(mutex);

    HADESMEM_DETAIL_TRACE_A("Direct3DCreate9ExHk called.");

    auto const d3d9_create_ex = g_d3d_mod.second->d3d9_create_ex_hk->
      GetTrampoline<decltype(&Direct3DCreate9ExHk)>();

    hr = d3d9_create_ex(sdk_version, ppd3d9);
    last_error = ::GetLastError();
    if (SUCCEEDED(hr) && ppd3d9 && *ppd3d9)
    {
      HADESMEM_DETAIL_TRACE_A("Direct3DCreate9Ex succeeded.");

      if (!g_d3d_mod.second->create_device_hk)
      {
        PBYTE* vmt = *reinterpret_cast<PBYTE**>(*ppd3d9);

        PBYTE create_device = vmt[16];
        PBYTE create_device_hk = reinterpret_cast<PBYTE>(
          reinterpret_cast<DWORD_PTR>(&CreateDeviceHk));

        g_d3d_mod.second->create_device_hk = 
          hadesmem::detail::make_unique<hadesmem::PatchDetour>(
          *g_process, create_device, create_device_hk);
        g_d3d_mod.second->create_device_hk->Apply();
      }
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("Direct3DCreate9Ex failed.");
    }
  }
  catch (std::exception const& e)
  {
    HADESMEM_DETAIL_TRACE_A(boost::diagnostic_information(e).c_str());
  }

  ::SetLastError(last_error);
  return hr;
}

extern "C" HRESULT WINAPI CreateDeviceHk(IDirect3D9* pd3d9, 
  UINT adapter, 
  D3DDEVTYPE device_type, 
  HWND focus_wnd, 
  DWORD behaviour_flags, 
  D3DPRESENT_PARAMETERS* presentation_params, 
  IDirect3DDevice9** ppdevice)
{
  HRESULT hr = static_cast<HRESULT>(0xDEADBEEF);
  DWORD last_error = 0xDEADBEEF;
  
  try
  {
    static boost::mutex mutex;
    boost::lock_guard<boost::mutex> lock(mutex);
    
    HADESMEM_DETAIL_TRACE_A("CreateDeviceHk called.");

    auto const create_device = g_d3d_mod.second->create_device_hk->
      GetTrampoline<decltype(&CreateDeviceHk)>();

    hr = create_device(pd3d9, adapter, device_type, focus_wnd, 
      behaviour_flags, presentation_params, ppdevice);
    last_error = ::GetLastError();
    if (SUCCEEDED(hr) && ppdevice && *ppdevice)
    {
      HADESMEM_DETAIL_TRACE_A("CreateDevice succeeded.");
      
      IDirect3DDevice9Proxy* proxy = new IDirect3DDevice9Proxy(*ppdevice);
      *ppdevice = proxy;
      (void)proxy;

      // TODO: Add code to clean up the proxy eventually. This would have to 
      // be done very carefully.
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("CreateDevice failed.");
    }
  }
  catch (std::exception const& e)
  {
    HADESMEM_DETAIL_TRACE_A(boost::diagnostic_information(e).c_str());
  }

  ::SetLastError(last_error);
  return hr;
}

// TODO: Fix this to properly support module unloads and loads, 
// then to support multiple modules simultaneously.
void ApplyDetours(HMODULE d3d9_mod)
{
  static boost::mutex mutex;
  boost::lock_guard<boost::mutex> lock(mutex);
  
  if (g_d3d_mod.first != nullptr && g_d3d_mod.first->GetHandle() != d3d9_mod)
  {
    HADESMEM_DETAIL_TRACE_A("Attempt to apply detours to a new module when "
      "the initial module is still loaded.");
    return;
  }

  if (g_d3d_mod.first != nullptr)
  {
    HADESMEM_DETAIL_TRACE_A("Reload of same D3D9 module. Ignoring.");
    return;
  }

  if (g_d3d_mod.first == nullptr)
  {
    HADESMEM_DETAIL_TRACE_A("New D3D9 module. Hooking.");

    HADESMEM_DETAIL_ASSERT(!g_d3d_mod.second);

    std::pair<std::unique_ptr<hadesmem::detail::SmartModuleHandle>, 
      std::unique_ptr<ModuleData>> d3d9_mod_tmp;
    d3d9_mod_tmp.first = 
      hadesmem::detail::make_unique<hadesmem::detail::SmartModuleHandle>();
    d3d9_mod_tmp.second = 
      hadesmem::detail::make_unique<ModuleData>();
    HMODULE d3d9_handle_tmp = nullptr;
    if (!::GetModuleHandleEx(
      GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, 
      reinterpret_cast<LPCTSTR>(d3d9_mod), 
      &d3d9_handle_tmp))
    {
      // TODO: Fix this. It currently doesn't work if we hook D3D9 as the 
      // result of a call to NtMapViewOfSection because the module isn't yet 
      // in the loader list... For now, just warn instead of failing.
      HADESMEM_DETAIL_TRACE_A("Warning! Attempt to bump ref count of D3D9 "
        "module failed.");
    }
    *d3d9_mod_tmp.first = d3d9_mod;
    
    FARPROC const d3d9_create = hadesmem::detail::FindProcedureInternal(
      *g_process, d3d9_mod, "Direct3DCreate9");
    if (d3d9_create)
    {
      PVOID target = reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
        d3d9_create));
      PVOID detour = reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
        &Direct3DCreate9Hk));
      d3d9_mod_tmp.second->d3d9_create_hk = 
        hadesmem::detail::make_unique<hadesmem::PatchDetour>(
        *g_process, target, detour);
      d3d9_mod_tmp.second->d3d9_create_hk->Apply();
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("Failed to find d3d9.dll!Direct3DCreate9.");
    }
    
    FARPROC const d3d9_create_ex = hadesmem::detail::FindProcedureInternal(
      *g_process, d3d9_mod, "Direct3DCreate9Ex");
    if (d3d9_create_ex)
    {
      PVOID target = reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
        d3d9_create_ex));
      PVOID detour = reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
        &Direct3DCreate9ExHk));
      d3d9_mod_tmp.second->d3d9_create_ex_hk = 
        hadesmem::detail::make_unique<hadesmem::PatchDetour>(
        *g_process, target, detour);
      d3d9_mod_tmp.second->d3d9_create_ex_hk->Apply();
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("Failed to find d3d9.dll!Direct3DCreate9Ex.");
    }

    g_d3d_mod = std::move(d3d9_mod_tmp);
  }
}

extern "C" NTSTATUS WINAPI NtMapViewOfSectionHk(
  HANDLE section, 
  HANDLE process, 
  PVOID* base, 
  ULONG_PTR zero_bits, 
  SIZE_T commit_size, 
  PLARGE_INTEGER section_offset, 
  PSIZE_T view_size, 
  HADESMEM_SECTION_INHERIT inherit_disposition, 
  ULONG alloc_type, 
  ULONG alloc_protect)
{
  NTSTATUS ret = static_cast<NTSTATUS>(0xDEADBEEF);
  DWORD last_error = 0xDEADBEEF;

  // Nothing must call NtMapViewOfSection until the the in_hook flag is set to 
  // true.
  // TODO: Replace the use of Boost with something custom so we can guarantee 
  // that in the future Boost won't start mapping sections.
  static boost::thread_specific_ptr<bool>* in_hook = new boost::thread_specific_ptr<bool>();
  if (in_hook->get() == nullptr)
  {
    in_hook->reset(new bool(false));
  }

  auto const nt_map_view_of_section = g_nt_map_view_of_section->
    GetTrampoline<decltype(&NtMapViewOfSectionHk)>();

  ret = nt_map_view_of_section(
    section, 
    process, 
    base, 
    zero_bits, 
    commit_size, 
    section_offset, 
    view_size, 
    inherit_disposition, 
    alloc_type, 
    alloc_protect);
  last_error = GetLastError();
  
  if (**in_hook == true)
  {
    ::SetLastError(last_error);
    return ret;
  }

  **in_hook = true;

  // TODO: Check whether GetProcessId could ever actually fail for the 
  // current process and find a workaround if it can.
  DWORD const pid = ::GetProcessId(process);
  if (!pid || pid != ::GetCurrentProcessId())
  {
    if (!pid)
    {
      HADESMEM_DETAIL_TRACE_A("NtMapViewOfSection called for unknown "
        "process.");
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("NtMapViewOfSection called for different "
        "process.");
    }
    **in_hook = false;
    ::SetLastError(last_error);
    return ret;
  }

  //HADESMEM_DETAIL_TRACE_A("NtMapViewOfSection called for current process.");

  try
  {
    if (NT_SUCCESS(ret))
    {
      //HADESMEM_DETAIL_TRACE_A("NtMapViewOfSection succeeded.");

      hadesmem::Region const region(*g_process, *base);
      DWORD const region_type = region.GetType();
      if (region_type != MEM_IMAGE)
      {
        //HADESMEM_DETAIL_TRACE_FORMAT_A("Not an image. Type given was %lx.", 
        //  region_type);
        **in_hook = false;
        ::SetLastError(last_error);
        return ret;
      }

      PVOID const arbitrary_user_pointer = 
        HadesMemCurrentTeb()->Tib.ArbitraryUserPointer;
      if (!arbitrary_user_pointer)
      {
        HADESMEM_DETAIL_TRACE_A("No arbitrary user pointer.");
        **in_hook = false;
        ::SetLastError(last_error);
        return ret;
      }

      std::wstring const path(static_cast<PCWSTR>(arbitrary_user_pointer));
      HADESMEM_DETAIL_TRACE_FORMAT_W(L"Path is %s.", path.c_str());

      auto const backslash = path.find_last_of(L'\\');
      if (backslash + 1 == L'\\')
      {
        HADESMEM_DETAIL_TRACE_A("Invalid path.");
        **in_hook = false;
        ::SetLastError(last_error);
        return ret;
      }
      std::size_t const name_beg_tmp = 
        (backslash != std::wstring::npos ? backslash + 1 : 0);
      HADESMEM_DETAIL_ASSERT(name_beg_tmp < static_cast<std::size_t>(
        (std::numeric_limits<std::wstring::difference_type>::max)()));
      auto const name_beg = static_cast<std::wstring::difference_type>(
        name_beg_tmp);
      std::wstring const module_name(std::begin(path) + name_beg, std::end(path));
      std::wstring const module_name_upper(
        hadesmem::detail::ToUpperOrdinal(module_name));
      if (module_name_upper == L"D3D9" || module_name_upper == L"D3D9.DLL")
      {
        HADESMEM_DETAIL_TRACE_A("D3D9 loaded. Applying hooks.");

        ApplyDetours(reinterpret_cast<HMODULE>(*base));
      }
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("NtMapViewOfSection failed.");
    }
  }
  catch (std::exception const& e)
  {
    HADESMEM_DETAIL_TRACE_A(boost::diagnostic_information(e).c_str());
  }

  **in_hook = false;

  ::SetLastError(last_error);
  return ret;
}

extern "C" BOOL WINAPI CreateProcessInternalWHk(
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
  BOOL ret = static_cast<BOOL>(0xDEADBEEF);
  DWORD last_error = 0xDEADBEEF;

  HADESMEM_DETAIL_TRACE_A("CreateProcessInternalW called.");

  // TODO: Log more details about params.

  try
  {
    auto const create_process_internal_w = g_create_process_internal_w->
      GetTrampoline<decltype(&CreateProcessInternalWHk)>();

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
      HADESMEM_DETAIL_TRACE_A("CreateProcessInternalW succeeded.");

      DWORD const pid = process_info->dwProcessId;
      HADESMEM_DETAIL_TRACE_FORMAT_W(L"PID 0n%lu", pid);

      HADESMEM_DETAIL_TRACE_A("Opening process.");
      hadesmem::Process const process(pid);
      HADESMEM_DETAIL_TRACE_A("Getting path to self.");
      std::wstring const self_path = hadesmem::detail::GetSelfPath();
      HADESMEM_DETAIL_TRACE_FORMAT_W(L"Path: \"%s\".", self_path.c_str());
      HADESMEM_DETAIL_TRACE_A("Injecting DLL.\n");
      HMODULE const remote_mod = hadesmem::InjectDll(process, self_path, 
        hadesmem::InjectFlags::kAddToSearchOrder);
      // TODO: Configurable export name
      HADESMEM_DETAIL_TRACE_A("Calling Load export.");
      hadesmem::CallResult<DWORD_PTR> init_result = hadesmem::CallExport(
        process, remote_mod, "Load");

      HADESMEM_DETAIL_TRACE_FORMAT_A(
        "Ret = 0x%Ix, LastError = 0x%lx", 
        init_result.GetReturnValue(), 
        init_result.GetLastError());
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("CreateProcessInternalW failed.");
    }
  }
  catch (std::exception const& e)
  {
    HADESMEM_DETAIL_TRACE_A(boost::diagnostic_information(e).c_str());
  }

  try
  {
    if (ret && (creation_flags & CREATE_SUSPENDED) == 0)
    {
      if (::ResumeThread(process_info->hThread) == static_cast<DWORD>(-1))
      {
        DWORD const last_error_inner = ::GetLastError();
        HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error() << 
          hadesmem::ErrorString("ResumeThread failed.") << 
          hadesmem::ErrorCodeWinLast(last_error_inner));
      }
    }
  }
  catch (std::exception const& e)
  {
    HADESMEM_DETAIL_TRACE_A(boost::diagnostic_information(e).c_str());

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
  
  g_process = hadesmem::detail::make_unique<hadesmem::Process>(
    ::GetCurrentProcessId());
  
  {
    HADESMEM_DETAIL_TRACE_A("Hooking CreateProcessInternalW.");

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
    g_create_process_internal_w = 
      hadesmem::detail::make_unique<hadesmem::PatchDetour>(
      *g_process, target, detour);
    g_create_process_internal_w->Apply();
  }
  
  try
  {
    hadesmem::Module d3d9_mod(*g_process, L"d3d9.dll");

    HADESMEM_DETAIL_TRACE_A("Hooking D3D9 directly.");

    ApplyDetours(d3d9_mod.GetHandle());
  }
  catch (std::exception const& /*e*/)
  {
    HADESMEM_DETAIL_TRACE_A("Hooking NtMapViewOfSection.");

    // TODO: Support multiple NtMapViewOfSection hooks.
    // TODO: Fall back gracefully if this API doesn't exist.
    hadesmem::Module ntdll(*g_process, L"ntdll.dll");
    FARPROC const nt_map_view_of_section = hadesmem::FindProcedure(
      *g_process, ntdll, "NtMapViewOfSection");
    PVOID target = reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
      nt_map_view_of_section));
    PVOID detour = reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
      &NtMapViewOfSectionHk));
    g_nt_map_view_of_section = 
      hadesmem::detail::make_unique<hadesmem::PatchDetour>(
      *g_process, target, detour);
    g_nt_map_view_of_section->Apply();

    return;
  }
}

void UninitializeD3D9Hooks()
{
  static boost::mutex mutex;
  boost::lock_guard<boost::mutex> lock(mutex);
  
  if (g_d3d_mod.first)
  {
    HADESMEM_DETAIL_TRACE_A("Cleaning up D3D mod handle.");
    g_d3d_mod.first = nullptr;
  }
  
  if (g_d3d_mod.second)
  {
    HADESMEM_DETAIL_TRACE_A("Cleaning up D3D mod data.");
    g_d3d_mod.second = nullptr;
  }
}

// TODO: Fix the code so this hack isn't required.
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic pop
#endif // #if defined(HADESMEM_CLANG)

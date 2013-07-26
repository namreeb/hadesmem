// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "d3d9mgr.hpp"

#include <mutex>
#include <memory>
#include <iostream>
#include <algorithm>

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
#include <hadesmem/detail/smart_handle.hpp>
#include <hadesmem/detail/static_assert.hpp>
#include <hadesmem/detail/to_upper_ordinal.hpp>

// TODO: Clean this all up.

// TODO: Add more error checking (e.g. all the D3DX calls).

// TODO: Thread safety for hooks.

// TODO: Add CreateProcess/ShellExecute/etc hooks. (Or perhaps at a lower 
// level? e.g. NtCreateUserProcess, NtCreateProcess, NtCreateProcessEx.)

// TODO: Support multiple D3D modules (both in parallel and reloading of a 
// single module).

// TODO: Find the right headers so that this can be removed.
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS (static_cast<NTSTATUS>(0x00000000L))
#endif
#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) ((static_cast<NTSTATUS>(Status)) >= 0)
#endif

namespace
{

// TODO: Fix the code so this hack isn't required.
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif // #if defined(HADESMEM_CLANG)

std::unique_ptr<hadesmem::detail::SmartModuleHandle> g_d3dx_mod;

std::unique_ptr<hadesmem::Process> g_process;

struct RenderData
{
  HWND g_wnd;
  IDirect3DStateBlock9* g_state_block;
  ID3DXLine* g_line;
  ID3DXFont* g_font;
};

std::map<IDirect3DDevice9*, RenderData> g_devices;

std::unique_ptr<hadesmem::PatchDetour> g_d3d9_create_hk;
std::unique_ptr<hadesmem::PatchDetour> g_d3d9_create_ex_hk;
std::unique_ptr<hadesmem::PatchDetour> g_create_device_hk;
std::unique_ptr<hadesmem::PatchDetour> g_end_scene_hk;
std::unique_ptr<hadesmem::PatchDetour> g_reset_hk;
std::unique_ptr<hadesmem::PatchDetour> g_release_hk;

std::unique_ptr<hadesmem::PatchDetour> g_ldr_load_dll;

// TODO: Fix the code so this hack isn't required.
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic pop
#endif // #if defined(HADESMEM_CLANG)

IDirect3D9* WINAPI Direct3DCreate9Hk(UINT sdk_version);

HRESULT WINAPI Direct3DCreate9ExHk(UINT sdk_version, IDirect3D9Ex** ppd3d9);

HRESULT WINAPI CreateDeviceHk(IDirect3D9* pd3d9, 
  UINT adapter, 
  D3DDEVTYPE device_type, 
  HWND focus_wnd, 
  DWORD behaviour_flags, 
  D3DPRESENT_PARAMETERS* presentation_params, 
  IDirect3DDevice9** ppdevice);

HRESULT WINAPI EndSceneHk(IDirect3DDevice9* device);

HRESULT WINAPI ResetHk(IDirect3DDevice9* device, 
  D3DPRESENT_PARAMETERS* presentation_params);

ULONG WINAPI ReleaseHk(IUnknown* unknown);

void OnEndScene(IDirect3DDevice9* device);

void OnPreReset(IDirect3DDevice9* device);

void OnPostReset(IDirect3DDevice9* device);

void OnRelease(IUnknown* device, ULONG ref_count);

void ApplyDetours(hadesmem::Module const& d3d9_mod);

NTSTATUS WINAPI LdrLoadDllHk(PCWSTR path, PULONG characteristics, 
  PCUNICODE_STRING name,  PVOID* handle);

// TODO: Thread safety
IDirect3D9* WINAPI Direct3DCreate9Hk(UINT sdk_version)
{
  ::OutputDebugStringA("Direct3DCreate9Hk called.\n");

  IDirect3D9* d3d9 = nullptr;
  DWORD last_error = 0;
  
  try
  {
    auto const d3d9_create = 
      reinterpret_cast<decltype(&Direct3DCreate9Hk)>(
      reinterpret_cast<DWORD_PTR>(
      g_d3d9_create_hk->GetTrampoline()));

    d3d9 = d3d9_create(sdk_version);
    last_error = ::GetLastError();
    if (d3d9)
    {
      ::OutputDebugStringA("Direct3DCreate9 succeeded.\n");

      if (!g_create_device_hk)
      {
        PBYTE* vmt = *reinterpret_cast<PBYTE**>(d3d9);

        PBYTE create_device = vmt[16];
        PBYTE create_device_hk = reinterpret_cast<PBYTE>(
          reinterpret_cast<DWORD_PTR>(&CreateDeviceHk));

        g_create_device_hk.reset(new hadesmem::PatchDetour(*g_process, 
          create_device, create_device_hk));
        g_create_device_hk->Apply();
      }
    }
    else
    {
      ::OutputDebugStringA("Direct3DCreate9 failed.\n");
    }
  }
  catch (std::exception const& e)
  {
    // TODO: Add proper logging.
    ::OutputDebugStringA((boost::diagnostic_information(e) + "\n").c_str());
  }

  ::SetLastError(last_error);
  return d3d9;
}

// TODO: Thread safety
HRESULT WINAPI Direct3DCreate9ExHk(UINT sdk_version, IDirect3D9Ex** ppd3d9)
{
  ::OutputDebugStringA("Direct3DCreate9ExHk called.\n");

  HRESULT hr = ERROR_NOT_SUPPORTED;
  DWORD last_error = 0;
  
  try
  {
    auto const d3d9_create_ex = 
      reinterpret_cast<decltype(&Direct3DCreate9ExHk)>(
      reinterpret_cast<DWORD_PTR>(
      g_d3d9_create_ex_hk->GetTrampoline()));

    hr = d3d9_create_ex(sdk_version, ppd3d9);
    last_error = ::GetLastError();
    if (SUCCEEDED(hr) && ppd3d9 && *ppd3d9)
    {
      ::OutputDebugStringA("Direct3DCreate9Ex succeeded.\n");

      if (!g_create_device_hk)
      {
        PBYTE* vmt = *reinterpret_cast<PBYTE**>(*ppd3d9);

        PBYTE create_device = vmt[16];
        PBYTE create_device_hk = reinterpret_cast<PBYTE>(
          reinterpret_cast<DWORD_PTR>(&CreateDeviceHk));

        g_create_device_hk.reset(new hadesmem::PatchDetour(*g_process, 
          create_device, create_device_hk));
        g_create_device_hk->Apply();
      }
    }
    else
    {
      ::OutputDebugStringA("Direct3DCreate9Ex failed.\n");
    }
  }
  catch (std::exception const& e)
  {
    // TODO: Add proper logging.
    ::OutputDebugStringA((boost::diagnostic_information(e) + "\n").c_str());
  }

  ::SetLastError(last_error);
  return hr;
}

// TODO: Thread safety
HRESULT WINAPI CreateDeviceHk(IDirect3D9* pd3d9, 
  UINT adapter, 
  D3DDEVTYPE device_type, 
  HWND focus_wnd, 
  DWORD behaviour_flags, 
  D3DPRESENT_PARAMETERS* presentation_params, 
  IDirect3DDevice9** ppdevice)
{
  ::OutputDebugStringA("CreateDeviceHk called.\n");

  HRESULT hr = ERROR_NOT_SUPPORTED;
  DWORD last_error_restored = 0;
  
  try
  {
    auto const create_device = 
      reinterpret_cast<decltype(&CreateDeviceHk)>(
      reinterpret_cast<DWORD_PTR>(
      g_create_device_hk->GetTrampoline()));

    hr = create_device(pd3d9, adapter, device_type, focus_wnd, 
      behaviour_flags, presentation_params, ppdevice);
    last_error_restored = ::GetLastError();
    if (SUCCEEDED(hr) && ppdevice && *ppdevice)
    {
      ::OutputDebugStringA("CreateDevice succeeded.\n");

      PBYTE* vmt = *reinterpret_cast<PBYTE**>(*ppdevice);

      if (!g_release_hk)
      {
        PBYTE release = vmt[2];
        PBYTE release_hk = reinterpret_cast<PBYTE>(
          reinterpret_cast<DWORD_PTR>(&ReleaseHk));

        g_release_hk.reset(new hadesmem::PatchDetour(*g_process, release, 
          release_hk));
        g_release_hk->Apply();
      }
    
      if (!g_reset_hk)
      {
        PBYTE reset = vmt[16];
        PBYTE reset_hk = reinterpret_cast<PBYTE>(
          reinterpret_cast<DWORD_PTR>(&ResetHk));

        g_reset_hk.reset(new hadesmem::PatchDetour(*g_process, reset, 
          reset_hk));
        g_reset_hk->Apply();
      }
    
      if (!g_end_scene_hk)
      {
        PBYTE end_scene = vmt[42];
        PBYTE end_scene_hk = reinterpret_cast<PBYTE>(
          reinterpret_cast<DWORD_PTR>(&EndSceneHk));

        g_end_scene_hk.reset(new hadesmem::PatchDetour(*g_process, 
          end_scene, end_scene_hk));
        g_end_scene_hk->Apply();
      }
      
      RenderData data;
      HADESMEM_STATIC_ASSERT(std::is_pod<RenderData>::value);
      ::ZeroMemory(&data, sizeof(data));

      try
      {
        auto device = *ppdevice;

        device->CreateStateBlock(D3DSBT_ALL, &data.g_state_block);

        if (!g_d3dx_mod)
        {
          g_d3dx_mod.reset(new hadesmem::detail::SmartModuleHandle(
            ::LoadLibraryW(L"d3dx9_43")));
        }

        if (!g_d3dx_mod->GetHandle())
        {
          DWORD const last_error = ::GetLastError();
          HADESMEM_THROW_EXCEPTION(hadesmem::Error() << 
            hadesmem::ErrorString("LoadLibraryW failed.") << 
            hadesmem::ErrorCodeWinLast(last_error));
        }
      
        auto d3dx_create_line = reinterpret_cast<decltype(&D3DXCreateLine)>(
          ::GetProcAddress(g_d3dx_mod->GetHandle(), "D3DXCreateLine"));
        if (!d3dx_create_line)
        {
          DWORD const last_error = ::GetLastError();
          HADESMEM_THROW_EXCEPTION(hadesmem::Error() << 
            hadesmem::ErrorString("GetProcAddress failed.") << 
            hadesmem::ErrorCodeWinLast(last_error));
        }
      
        HRESULT line_hr = d3dx_create_line(device, &data.g_line);
        if (FAILED(line_hr))
        {
          BOOST_THROW_EXCEPTION(hadesmem::Error() << 
            hadesmem::ErrorString("Failed to create line.") << 
            hadesmem::ErrorCodeWinHr(line_hr));
        }

        std::wstring const face_name(L"Tahoma");
        DWORD const height = 10;
      
        auto d3dx_create_font = reinterpret_cast<decltype(&D3DXCreateFontW)>(
          ::GetProcAddress(g_d3dx_mod->GetHandle(), "D3DXCreateFontW"));
        if (!d3dx_create_font)
        {
          DWORD const last_error = ::GetLastError();
          HADESMEM_THROW_EXCEPTION(hadesmem::Error() << 
            hadesmem::ErrorString("GetProcAddress failed.") << 
            hadesmem::ErrorCodeWinLast(last_error));
        }
      
        HRESULT font_hr = d3dx_create_font(
          device, 
          -MulDiv(height, GetDeviceCaps(GetDC(0), LOGPIXELSY), 72), 
          0, 
          FW_NORMAL, 
          0, 
          0, 
          DEFAULT_CHARSET, 
          OUT_DEFAULT_PRECIS, 
          DEFAULT_QUALITY, 
          DEFAULT_PITCH | FF_DONTCARE, 
          face_name.c_str(), 
          &data.g_font);
        if (FAILED(font_hr))
        {
          BOOST_THROW_EXCEPTION(hadesmem::Error() << 
            hadesmem::ErrorString("Failed to create font.") << 
            hadesmem::ErrorCodeWinHr(font_hr));
        }
      
        data.g_font->PreloadCharacters(0, 255);

        g_devices[device] = data;
      }
      catch (std::exception const& /*e*/)
      {
        ::OutputDebugStringA("Failed while initializing render data.\n");

        data.g_state_block->Release();
        data.g_state_block = nullptr;
  
        data.g_line->Release();
        data.g_line = nullptr;
  
        data.g_font->Release();
        data.g_font = nullptr;

        throw;
      }
    }
    else
    {
      ::OutputDebugStringA("CreateDevice failed.\n");
    }
  }
  catch (std::exception const& e)
  {
    // TODO: Add proper logging.
    ::OutputDebugStringA((boost::diagnostic_information(e) + "\n").c_str());
  }

  ::SetLastError(last_error_restored);
  return hr;
}

HRESULT WINAPI EndSceneHk(IDirect3DDevice9* device)
{
  auto const end_scene = 
    reinterpret_cast<decltype(&EndSceneHk)>(
    reinterpret_cast<DWORD_PTR>(
    g_end_scene_hk->GetTrampoline()));
  
  try
  {
    OnEndScene(device);
  }
  catch (std::exception const& e)
  {
    // TODO: Add proper logging.
    ::OutputDebugStringA((boost::diagnostic_information(e) + "\n").c_str());
  }

  return end_scene(device);
}

HRESULT WINAPI ResetHk(IDirect3DDevice9* device, 
  D3DPRESENT_PARAMETERS* presentation_params)
{
  ::OutputDebugStringA("Reset called.\n");

  auto const reset = 
    reinterpret_cast<decltype(&ResetHk)>(
    reinterpret_cast<DWORD_PTR>(
    g_reset_hk->GetTrampoline()));

  try
  {
    OnPreReset(device);
  }
  catch (std::exception const& e)
  {
    // TODO: Add proper logging.
    ::OutputDebugStringA((boost::diagnostic_information(e) + "\n").c_str());
  }

  HRESULT hr = reset(device, presentation_params);
  DWORD const last_error = ::GetLastError();
  
  ::OutputDebugStringA(SUCCEEDED(hr) ? 
    "Reset succeeded.\n" : 
    "Reset failed.\n");

  try
  {
    OnPostReset(device);
  }
  catch (std::exception const& e)
  {
    // TODO: Add proper logging.
    ::OutputDebugStringA((boost::diagnostic_information(e) + "\n").c_str());
  }

  ::SetLastError(last_error);
  return hr;
}

ULONG WINAPI ReleaseHk(IUnknown* unknown)
{
  auto const release = 
    reinterpret_cast<decltype(&ReleaseHk)>(
    reinterpret_cast<DWORD_PTR>(
    g_release_hk->GetTrampoline()));
  
  ULONG ref_count = release(unknown);
  DWORD const last_error = ::GetLastError();
  
  try
  {
    OnRelease(unknown, ref_count);
  }
  catch (std::exception const& e)
  {
    // TODO: Add proper logging.
    ::OutputDebugStringA((boost::diagnostic_information(e) + "\n").c_str());
  }

  ::SetLastError(last_error);
  return ref_count;
}

void OnEndScene(IDirect3DDevice9* device)
{
  // TODO: Thread safety.
  auto iter = g_devices.find(device);
  if (iter == g_devices.end())
  {
    return;
  }

  RenderData const& data = iter->second;

  data.g_state_block->Capture();

  auto const draw_line = [&] (D3DXVECTOR2 start, D3DXVECTOR2 end, 
    float width, D3DCOLOR colour)
  {
    data.g_line->SetWidth(width);

    D3DXVECTOR2 vec[2] = 
    {
      start, end
    };

    data.g_line->Begin();
    data.g_line->Draw(vec, 2, colour);
    data.g_line->End();
  };

  auto const draw_box = [&] (D3DXVECTOR2 bottom_left, D3DXVECTOR2 top_right, 
    float line_width, D3DCOLOR colour)
  {
    float width = top_right[0] - bottom_left[0];
    float height = top_right[1] - bottom_left[1];

    D3DXVECTOR2 top_left(top_right[0] - width, top_right[1]);
    D3DXVECTOR2 bottom_right(top_right[0], top_right[1] - height);

    draw_line(bottom_left, top_left, line_width, colour);
    draw_line(bottom_left, bottom_right, line_width, colour);
    draw_line(bottom_right, top_right, line_width, colour);
    draw_line(top_left, top_right, line_width, colour);
  };
  
  D3DVIEWPORT9 viewport;
  device->GetViewport(&viewport);

  D3DXVECTOR2 const top_left(1,1);
  D3DXVECTOR2 const bottom_right(static_cast<float>(viewport.Width) - 1, 
    static_cast<float>(viewport.Height) - 1);
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshift-sign-overflow"
#endif // #if defined(HADESMEM_CLANG)
  draw_box(top_left, bottom_right, 5, D3DCOLOR_ARGB(255, 0, 255, 0));
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic pop
#endif // #if defined(HADESMEM_CLANG)

  std::wstring const text(L"Hades");
  LONG const text_draw_x = 5;
  LONG const text_draw_y = 5;
  RECT text_draw_rect = { text_draw_x, text_draw_y, 0, 0 };
  data.g_font->DrawText(
    nullptr, 
    text.c_str(), 
    -1, 
    &text_draw_rect, 
    DT_NOCLIP, 
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshift-sign-overflow"
#endif // #if defined(HADESMEM_CLANG)
    D3DCOLOR_RGBA(0, 255, 0, 255));
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic pop
#endif // #if defined(HADESMEM_CLANG)
  
  data.g_state_block->Apply();
}

void OnPreReset(IDirect3DDevice9* device)
{
  // TODO: Thread safety.
  auto iter = g_devices.find(device);
  if (iter == g_devices.end())
  {
    return;
  }

  RenderData& data = iter->second;
  
  if (data.g_state_block)
  {
    data.g_state_block->Release();
    data.g_state_block = nullptr;
  }
  else
  {
    ::OutputDebugStringA("Warning! OnPreReset called with invalid render data"
      " (state block).\n");
  }

  if (data.g_line)
  {
    data.g_line->OnLostDevice();
  }
  else
  {
    ::OutputDebugStringA("Warning! OnPreReset called with invalid render data"
      " (line).\n");
  }

  if (data.g_font)
  {
    data.g_font->OnLostDevice();
  }
  else
  {
    ::OutputDebugStringA("Warning! OnPreReset called with invalid render data"
      " (font).\n");
  }
}

void OnPostReset(IDirect3DDevice9* device)
{
  // TODO: Thread safety.
  auto iter = g_devices.find(device);
  if (iter == g_devices.end())
  {
    return;
  }

  RenderData& data = iter->second;
  
  if (data.g_state_block)
  {
    ::OutputDebugStringA("Warning! OnPostReset called with invalid render "
      "data (state block).\n");
  }
  else
  {
    device->CreateStateBlock(D3DSBT_ALL, &data.g_state_block);
  }

  if (data.g_line)
  {
    data.g_line->OnResetDevice();
  }
  else
  {
    ::OutputDebugStringA("Warning! OnPostReset called with invalid render "
      "data (line).\n");
  }

  if (data.g_font)
  {
    data.g_font->OnResetDevice();
  }
  else
  {
    ::OutputDebugStringA("Warning! OnPostReset called with invalid render "
      "data (font).\n");
  }
}

void OnRelease(IUnknown* device, ULONG ref_count)
{
  if (ref_count != 0)
  {
    return;
  }

  // TODO: Thread safety.
  auto iter = g_devices.find(static_cast<IDirect3DDevice9*>(device));
  if (iter == g_devices.end())
  {
    return;
  }

  ::OutputDebugStringA("OnRelease actually cleaning up!\n");

  RenderData& data = iter->second;
  
  if (data.g_state_block)
  {
    data.g_state_block->Release();
    data.g_state_block = nullptr;
  }
  
  if (data.g_line)
  {
    data.g_line->Release();
    data.g_line = nullptr;
  }
  
  if (data.g_font)
  {
    data.g_font->Release();
    data.g_font = nullptr;
  }
  
  g_devices.erase(iter);
}

void ApplyDetours(hadesmem::Module const& d3d9_mod)
{
  try
  {
    // TODO: Thread safety
    if (!g_d3d9_create_hk)
    {
      FARPROC const d3d9_create = hadesmem::FindProcedure(*g_process, d3d9_mod, 
        "Direct3DCreate9");
      PVOID target = reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
        d3d9_create));
      PVOID detour = reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
        &Direct3DCreate9Hk));
      g_d3d9_create_hk.reset(new hadesmem::PatchDetour(*g_process, target, 
        detour));
      g_d3d9_create_hk->Apply();
    }

    // TODO: Thread safety
    if (!g_d3d9_create_ex_hk)
    {
      // TODO: Fail gracefully when this function is unavailable. (Investigate 
      // when this can happen... Maybe it's unnecessary nowadays?)
      FARPROC const d3d9_create_ex = hadesmem::FindProcedure(*g_process, 
        d3d9_mod, "Direct3DCreate9Ex");
      PVOID target = reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
        d3d9_create_ex));
      PVOID detour = reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
        &Direct3DCreate9ExHk));
      g_d3d9_create_ex_hk.reset(new hadesmem::PatchDetour(*g_process, target, 
        detour));
      g_d3d9_create_ex_hk->Apply();
    }
  }
  catch (std::exception const& e)
  {
    ::OutputDebugStringA((boost::diagnostic_information(e) + "\n").c_str());
  }
}

NTSTATUS WINAPI LdrLoadDllHk(PCWSTR path, PULONG characteristics, 
  PCUNICODE_STRING name,  PVOID* handle)
{
  ::OutputDebugStringA("LdrLoadDll called.\n");

  NTSTATUS ret = STATUS_SUCCESS;
  DWORD last_error = 0;
  
  try
  {
    auto const ldr_load_dll = 
      reinterpret_cast<decltype(&LdrLoadDllHk)>(
      reinterpret_cast<DWORD_PTR>(
      g_ldr_load_dll->GetTrampoline()));

    ret = ldr_load_dll(path, characteristics, name, handle);
    last_error = GetLastError();
    if (NT_SUCCESS(ret))
    {
      ::OutputDebugStringA("LdrLoadDll succeeded.\n");

      if (name && name->Length && name->Buffer)
      {
        std::wstring const name_real(hadesmem::detail::ToUpperOrdinal(
          std::wstring(name->Buffer, name->Buffer + name->Length)));
        OutputDebugStringW((L"Loaded DLL: " + name_real + L".\n").c_str());
        std::wstring const d3d9_dll_name(hadesmem::detail::ToUpperOrdinal(
          L"d3d9.dll"));
        std::wstring const d3d9_name(hadesmem::detail::ToUpperOrdinal(
          L"d3d9"));
        if (name_real == d3d9_dll_name || name_real == d3d9_name)
        {
          ::OutputDebugStringA("D3D9 loaded. Applying hooks.\n");

          hadesmem::Module const d3d9_mod(*g_process, 
            reinterpret_cast<HMODULE>(*handle));
          ApplyDetours(d3d9_mod);
        }
      }
    }
    else
    {
      ::OutputDebugStringA("LdrLoadDll failed.\n");
    }
  }
  catch (std::exception const& e)
  {
    // TODO: Add proper logging.
    ::OutputDebugStringA((boost::diagnostic_information(e) + "\n").c_str());
  }

  ::SetLastError(last_error);
  return ret;
}

}

void InitializeD3D9Hooks()
{
  g_process.reset(new hadesmem::Process(::GetCurrentProcessId()));
  
  std::unique_ptr<hadesmem::Module> d3d9_mod;
  try
  {
    d3d9_mod.reset(new hadesmem::Module(*g_process, L"d3d9.dll"));
  }
  catch (std::exception const& /*e*/)
  {
    ::OutputDebugStringA("Hooking LdrLoadDll.\n");

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

  ::OutputDebugStringA("Hooking D3D9 directly.\n");

  ApplyDetours(*d3d9_mod);
}

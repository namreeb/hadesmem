// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include <cstdint>
#include <iostream>

#include <windows.h>

#include <d3d9.h>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/detail/scope_warden.hpp>
#include <hadesmem/detail/smart_handle.hpp>
#include <hadesmem/detail/str_conv.hpp>

#include <cerberus/d3d9.hpp>

int main(int argc, char* argv[])
{
  try
  {
    std::cout << "HadesMem Cerberus Helper [" << HADESMEM_VERSION_STRING
              << "]\n";

    if (argc != 2)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error{}
                                      << hadesmem::ErrorString{
                                        "Invalid number of arguments."});
    }

    auto const d3d9_mod = LoadLibraryW(L"d3d9.dll");
    if (!d3d9_mod)
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"LoadLibraryW failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }

    auto const wnd = CreateWindowExW(0,
                                     L"STATIC",
                                     L"Cerberus D3D9 Winndow",
                                     WS_POPUP,
                                     0,
                                     0,
                                     1,
                                     1,
                                     nullptr,
                                     nullptr,
                                     nullptr,
                                     nullptr);
    if (!wnd)
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"CreateWindowExW failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }

    auto const destroy_window = [&]()
    {
      if (!::DestroyWindow(wnd))
      {
        DWORD const last_error = ::GetLastError();
        HADESMEM_DETAIL_TRACE_FORMAT_A(
          "WARNING! DestroyWindow failed. LastError = [%lu].", last_error);
      }
    };

    auto ensure_destroy_window =
      hadesmem::detail::MakeScopeWarden(destroy_window);

    auto const d3d9_create_ex = reinterpret_cast<decltype(&Direct3DCreate9Ex)>(
      ::GetProcAddress(d3d9_mod, "Direct3DCreate9Ex"));
    if (!d3d9_create_ex)
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"GetProcAddress failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }

    IDirect3D9Ex* d3d9_ex = nullptr;
    auto const create_d3d9_ex_hr = d3d9_create_ex(D3D_SDK_VERSION, &d3d9_ex);
    if (FAILED(create_d3d9_ex_hr))
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"Direct3DCreate9Ex failed."}
                          << hadesmem::ErrorCodeWinHr{create_d3d9_ex_hr});
    }

    hadesmem::detail::SmartComHandle smart_d3d9_ex{d3d9_ex};

    D3DPRESENT_PARAMETERS pp = {};
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_FLIP;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8;
    pp.BackBufferWidth = 2;
    pp.BackBufferHeight = 2;
    pp.BackBufferCount = 1;
    pp.hDeviceWindow = wnd;
    pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    IDirect3DDevice9Ex* device_ex = nullptr;
    auto const create_device_hr = d3d9_ex->CreateDeviceEx(
      D3DADAPTER_DEFAULT,
      D3DDEVTYPE_HAL,
      wnd,
      D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_NOWINDOWCHANGES,
      &pp,
      nullptr,
      &device_ex);
    if (FAILED(create_device_hr))
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{}
        << hadesmem::ErrorString{"IDirect3D9Ex::CreateDeviceEx failed."}
        << hadesmem::ErrorCodeWinHr{create_d3d9_ex_hr});
    }

    hadesmem::detail::SmartComHandle smart_device_ex{device_ex};

    IDirect3DSwapChain9* swap_chain = nullptr;
    auto const get_swap_chain_hr = device_ex->GetSwapChain(0, &swap_chain);
    if (FAILED(get_swap_chain_hr))
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{}
        << hadesmem::ErrorString{"IDirect3D9Ex::GetSwapChain failed."}
        << hadesmem::ErrorCodeWinHr{create_d3d9_ex_hr});
    }

    hadesmem::detail::SmartComHandle smart_swap_chain{swap_chain};

    auto const add_ref_fn = (*reinterpret_cast<void***>(device_ex))[1];
    auto const release_fn = (*reinterpret_cast<void***>(device_ex))[2];
    auto const present_fn = (*reinterpret_cast<void***>(device_ex))[17];
    auto const reset_fn = (*reinterpret_cast<void***>(device_ex))[16];
    auto const end_scene_fn = (*reinterpret_cast<void***>(device_ex))[42];
    auto const present_ex_fn = (*reinterpret_cast<void***>(device_ex))[121];
    auto const reset_ex_fn = (*reinterpret_cast<void***>(device_ex))[132];
    auto const swap_chain_present_fn =
      (*reinterpret_cast<void***>(swap_chain))[3];

    auto const file_mapping_name =
      CERBERUS_HELPER_D3D9_MAP_NAME +
      hadesmem::detail::MultiByteToWideChar(argv[1]);
    HADESMEM_DETAIL_TRACE_FORMAT_W(L"Helper mapping name: [%s].",
                                   file_mapping_name.c_str());
    hadesmem::detail::SmartHandle file_mapping{
      ::OpenFileMappingW(FILE_MAP_WRITE, FALSE, file_mapping_name.c_str())};
    if (!file_mapping.IsValid())
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"OpenFileMappingW failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }

    hadesmem::detail::SmartMappedFileHandle mapping_view{::MapViewOfFileEx(
      file_mapping.GetHandle(), FILE_MAP_WRITE, 0, 0, 0, nullptr)};
    if (!mapping_view.IsValid())
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"MapViewOfFileEx failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }

    // TODO: Ensure we're not being shimmed/hooked/etc or the offsets might not
    // be valid in the target process.
    auto const d3d9_offsets =
      static_cast<hadesmem::cerberus::D3D9Offsets*>(mapping_view.GetHandle());
    auto const base = reinterpret_cast<std::uintptr_t>(d3d9_mod);
    d3d9_offsets->add_ref_ =
      reinterpret_cast<std::uintptr_t>(add_ref_fn) - base;
    d3d9_offsets->release_ =
      reinterpret_cast<std::uintptr_t>(release_fn) - base;
    d3d9_offsets->present_ =
      reinterpret_cast<std::uintptr_t>(present_fn) - base;
    d3d9_offsets->reset_ = reinterpret_cast<std::uintptr_t>(reset_fn) - base;
    d3d9_offsets->end_scene_ =
      reinterpret_cast<std::uintptr_t>(end_scene_fn) - base;
    d3d9_offsets->present_ex_ =
      reinterpret_cast<std::uintptr_t>(present_ex_fn) - base;
    d3d9_offsets->reset_ex_ =
      reinterpret_cast<std::uintptr_t>(reset_ex_fn) - base;
    d3d9_offsets->swap_chain_present_ =
      reinterpret_cast<std::uintptr_t>(swap_chain_present_fn) - base;

    return 0;
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());

    std::cerr << "\nError!\n";
    std::cerr << boost::current_exception_diagnostic_information() << '\n';

    return 1;
  }
}

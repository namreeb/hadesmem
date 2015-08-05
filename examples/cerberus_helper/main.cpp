// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include <cstdint>
#include <iostream>

#include <windows.h>

#include <d3d9.h>

#include <d3d11.h>

#include <dxgi1_2.h>
#include <dxgi.h>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/detail/scope_warden.hpp>
#include <hadesmem/detail/smart_handle.hpp>
#include <hadesmem/detail/str_conv.hpp>

#include <cerberus/render_helper.hpp>

hadesmem::cerberus::D3D9Offsets GetD3D9Offsets()
{
  hadesmem::detail::SmartModuleHandle d3d9_mod{::LoadLibraryW(L"d3d9.dll")};
  if (!d3d9_mod.IsValid())
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"LoadLibraryW failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  auto const wnd = ::CreateWindowExW(0,
                                     L"STATIC",
                                     L"Cerberus D3D9 Window",
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

  auto const direct3d_create_9_ex =
    reinterpret_cast<decltype(&Direct3DCreate9Ex)>(
      ::GetProcAddress(d3d9_mod.GetHandle(), "Direct3DCreate9Ex"));
  if (!direct3d_create_9_ex)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"GetProcAddress failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  IDirect3D9Ex* d3d9_ex = nullptr;
  auto const create_d3d9_ex_hr =
    direct3d_create_9_ex(D3D_SDK_VERSION, &d3d9_ex);
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

  HADESMEM_DETAIL_TRACE_FORMAT_A("IDirect3D9Ex::AddRef: %p.", add_ref_fn);
  HADESMEM_DETAIL_TRACE_FORMAT_A("IDirect3D9Ex::Release: %p.", release_fn);
  HADESMEM_DETAIL_TRACE_FORMAT_A("IDirect3D9Ex::Present: %p.", present_fn);
  HADESMEM_DETAIL_TRACE_FORMAT_A("IDirect3D9Ex::Reset: %p.", reset_fn);
  HADESMEM_DETAIL_TRACE_FORMAT_A("IDirect3D9Ex::EndScene: %p.", end_scene_fn);
  HADESMEM_DETAIL_TRACE_FORMAT_A("IDirect3D9Ex::PresentEx: %p.", present_ex_fn);
  HADESMEM_DETAIL_TRACE_FORMAT_A("IDirect3D9Ex::ResetEx: %p.", reset_ex_fn);

  auto const swap_chain_present_fn =
    (*reinterpret_cast<void***>(swap_chain))[3];

  HADESMEM_DETAIL_TRACE_FORMAT_A("IDirect3DSwapChain9::Present: %p.",
                                 swap_chain_present_fn);

  hadesmem::cerberus::D3D9Offsets d3d9_offsets = {};

  auto const base = reinterpret_cast<std::uintptr_t>(d3d9_mod.GetHandle());
  d3d9_offsets.add_ref_ = reinterpret_cast<std::uintptr_t>(add_ref_fn) - base;
  d3d9_offsets.release_ = reinterpret_cast<std::uintptr_t>(release_fn) - base;
  d3d9_offsets.present_ = reinterpret_cast<std::uintptr_t>(present_fn) - base;
  d3d9_offsets.reset_ = reinterpret_cast<std::uintptr_t>(reset_fn) - base;
  d3d9_offsets.end_scene_ =
    reinterpret_cast<std::uintptr_t>(end_scene_fn) - base;
  d3d9_offsets.present_ex_ =
    reinterpret_cast<std::uintptr_t>(present_ex_fn) - base;
  d3d9_offsets.reset_ex_ = reinterpret_cast<std::uintptr_t>(reset_ex_fn) - base;
  d3d9_offsets.swap_chain_present_ =
    reinterpret_cast<std::uintptr_t>(swap_chain_present_fn) - base;

  return d3d9_offsets;
}

hadesmem::cerberus::DXGIOffsets GetDXGIOffsets()
{
  hadesmem::detail::SmartModuleHandle dxgi_mod{::LoadLibraryW(L"dxgi.dll")};
  if (!dxgi_mod.IsValid())
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"LoadLibraryW failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  hadesmem::detail::SmartModuleHandle d3d11_mod{::LoadLibraryW(L"d3d11.dll")};
  if (!d3d11_mod.IsValid())
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"LoadLibraryW failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  auto const wnd = ::CreateWindowExW(0,
                                     L"STATIC",
                                     L"Cerberus DXGI Window",
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

  auto const d3d11_create_device =
    reinterpret_cast<decltype(&D3D11CreateDevice)>(
      ::GetProcAddress(d3d11_mod.GetHandle(), "D3D11CreateDevice"));
  if (!d3d11_create_device)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"GetProcAddress failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  ID3D11Device* d3d11_device = nullptr;
  auto const d3d11_create_device_hr =
    d3d11_create_device(nullptr,
                        D3D_DRIVER_TYPE_HARDWARE,
                        nullptr,
                        0,
                        nullptr,
                        0,
                        D3D11_SDK_VERSION,
                        &d3d11_device,
                        nullptr,
                        nullptr);
  if (FAILED(d3d11_create_device_hr))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"D3D11CreateDevice failed."}
                        << hadesmem::ErrorCodeWinHr{d3d11_create_device_hr});
  }

  hadesmem::detail::SmartComHandle smart_d3d11_device{d3d11_device};

  auto const create_dxgi_factory =
    reinterpret_cast<decltype(&CreateDXGIFactory)>(
      ::GetProcAddress(dxgi_mod.GetHandle(), "CreateDXGIFactory"));
  if (!d3d11_create_device)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"GetProcAddress failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  IDXGIFactory2* dxgi_factory = nullptr;
  auto const create_dxgi_factory_hr = create_dxgi_factory(
    __uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgi_factory));
  if (FAILED(create_dxgi_factory_hr))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"CreateDXGIFactory failed."}
                        << hadesmem::ErrorCodeWinHr{create_dxgi_factory_hr});
  }

  hadesmem::detail::SmartComHandle smart_dxgi_factory{dxgi_factory};

  DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
  swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swap_chain_desc.BufferCount = 2;
  swap_chain_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
  swap_chain_desc.SampleDesc.Count = 1;

  IDXGISwapChain1* dxgi_swap_chain = nullptr;
  auto const create_swap_chain_hr = dxgi_factory->CreateSwapChainForHwnd(
    d3d11_device, wnd, &swap_chain_desc, nullptr, nullptr, &dxgi_swap_chain);
  if (FAILED(create_swap_chain_hr))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{}
      << hadesmem::ErrorString{"IDXGIFactory2::CreateSwapChainForHwnd failed."}
      << hadesmem::ErrorCodeWinHr{create_swap_chain_hr});
  }

  hadesmem::detail::SmartComHandle smart_dxgi_swap_chain{dxgi_swap_chain};

  auto const present_fn = (*reinterpret_cast<void***>(dxgi_swap_chain))[8];
  auto const resize_buffers_fn =
    (*reinterpret_cast<void***>(dxgi_swap_chain))[13];
  auto const present_1_fn = (*reinterpret_cast<void***>(dxgi_swap_chain))[22];

  HADESMEM_DETAIL_TRACE_FORMAT_A("IDXGISwapChain1::Present: %p.", present_fn);
  HADESMEM_DETAIL_TRACE_FORMAT_A("IDXGISwapChain1::ResizeBuffers: %p.",
                                 resize_buffers_fn);
  HADESMEM_DETAIL_TRACE_FORMAT_A("IDXGISwapChain1::Present1: %p.",
                                 present_1_fn);

  hadesmem::cerberus::DXGIOffsets dxgi_offsets = {};

  auto const base = reinterpret_cast<std::uintptr_t>(dxgi_mod.GetHandle());
  dxgi_offsets.present_ = reinterpret_cast<std::uintptr_t>(present_fn) - base;
  dxgi_offsets.resize_buffers_ =
    reinterpret_cast<std::uintptr_t>(resize_buffers_fn) - base;
  dxgi_offsets.present_1_ =
    reinterpret_cast<std::uintptr_t>(present_1_fn) - base;

  return dxgi_offsets;
}

// TODO: Ensure we're not being shimmed/hooked/etc or the offsets might not
// be valid in the target process.
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

    auto const file_mapping_name =
      hadesmem::cerberus::GenerateRenderHelperMapName(
        hadesmem::detail::MultiByteToWideChar(argv[1]));
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

    auto const render_offsets =
      static_cast<hadesmem::cerberus::RenderOffsets*>(mapping_view.GetHandle());

    render_offsets->d3d9_offsets_ = GetD3D9Offsets();
    render_offsets->dxgi_offsets_ = GetDXGIOffsets();

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

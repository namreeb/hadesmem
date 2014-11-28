// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "dxgi_helpers.hpp"

#include <dxgi1_2.h>
#include <dxgi.h>

#include <hadesmem/detail/trace.hpp>

namespace hadesmem
{
namespace cerberus
{
DXGIFactoryWrapper GetDXGIFactoryFromDevice(IUnknown* device)
{
  IDXGIDevice* dxgi_device = nullptr;
  IDXGIAdapter* dxgi_adapter = nullptr;
  IDXGIFactory* dxgi_factory = nullptr;
  std::uint32_t factory_revision = 2;

  try
  {
    if (FAILED(device->QueryInterface(__uuidof(IDXGIDevice),
                                      reinterpret_cast<void**>(&dxgi_device))))
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{
                               "ID3D11Device::QueryInterface failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }

    if (FAILED(dxgi_device->GetParent(__uuidof(IDXGIAdapter),
                                      reinterpret_cast<void**>(&dxgi_adapter))))
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{
                               "IDXGIDevice::GetParent failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }

    if (FAILED(dxgi_adapter->GetParent(
          __uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgi_factory))))
    {
      --factory_revision;
      if (FAILED(dxgi_adapter->GetParent(
            __uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgi_factory))))
      {
        --factory_revision;
        if (FAILED(dxgi_adapter->GetParent(
              __uuidof(IDXGIFactory), reinterpret_cast<void**>(&dxgi_factory))))
        {
          DWORD const last_error = ::GetLastError();
          HADESMEM_DETAIL_THROW_EXCEPTION(
            hadesmem::Error{}
            << hadesmem::ErrorString{"IDXGIAdapter::GetParent failed."}
            << hadesmem::ErrorCodeWinLast{last_error});
        }
      }
    }
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }

  return {dxgi_device, dxgi_adapter, dxgi_factory, factory_revision};
}
}
}

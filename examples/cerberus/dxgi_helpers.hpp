// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>

#include <dxgi.h>

#include <hadesmem/config.hpp>

namespace hadesmem
{

namespace cerberus
{

class DXGIFactoryWrapper
{
public:
  DXGIFactoryWrapper(IDXGIDevice* device,
                     IDXGIAdapter* adapter,
                     IDXGIFactory* factory,
                     std::uint32_t factory_revision)
    : device_{device},
      adapter_{adapter},
      factory_{factory},
      factory_revision_{factory_revision}
  {
  }

  DXGIFactoryWrapper(DXGIFactoryWrapper const&) = delete;

  DXGIFactoryWrapper& operator=(DXGIFactoryWrapper const&) = delete;

#if defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

  DXGIFactoryWrapper(DXGIFactoryWrapper&& other) HADESMEM_DETAIL_NOEXCEPT
    : device_(other.device_),
      adapter_(other.adapter_),
      factory_(other.factory_)
  {
    other.device_ = nullptr;
    other.adapter_ = nullptr;
    other.factory_ = nullptr;
  }

  DXGIFactoryWrapper& operator=(DXGIFactoryWrapper&& other)
    HADESMEM_DETAIL_NOEXCEPT
  {
    Cleanup();

    device_ = nullptr;
    adapter_ = nullptr;
    factory_ = nullptr;

    std::swap(device_, other.device_);
    std::swap(adapter_, other.adapter_);
    std::swap(factory_, other.factory_);

    return *this;
  }

#else

  DXGIFactoryWrapper(DXGIFactoryWrapper&& other) = default;

  DXGIFactoryWrapper& operator=(DXGIFactoryWrapper&& other) = default;

#endif // #if defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

  ~DXGIFactoryWrapper()
  {
    Cleanup();
  }

  void Cleanup() HADESMEM_DETAIL_NOEXCEPT
  {
    if (factory_)
    {
      factory_->Release();
    }

    if (adapter_)
    {
      adapter_->Release();
    }

    if (device_)
    {
      device_->Release();
    }
  }

  IDXGIFactory* GetFactory() const HADESMEM_DETAIL_NOEXCEPT
  {
    return factory_;
  }

  std::uint32_t GetFactoryRevision() const HADESMEM_DETAIL_NOEXCEPT
  {
    return factory_revision_;
  }

private:
  IDXGIDevice* device_{};
  IDXGIAdapter* adapter_{};
  IDXGIFactory* factory_{};
  std::uint32_t factory_revision_{};
};

DXGIFactoryWrapper GetDXGIFactoryFromDevice(IUnknown* device);
}
}

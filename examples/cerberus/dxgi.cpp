// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "dxgi.hpp"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <iterator>
#include <memory>
#include <mutex>
#include <string>

#include <windows.h>
#include <winnt.h>
#include <winternl.h>

#include <dxgi.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/detour_ref_counter.hpp>
#include <hadesmem/detail/last_error_preserver.hpp>
#include <hadesmem/detail/winternl.hpp>
#include <hadesmem/find_procedure.hpp>
#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/region.hpp>

#include "callbacks.hpp"
#include "helpers.hpp"
#include "main.hpp"
#include "module.hpp"

namespace
{

std::unique_ptr<hadesmem::PatchDetour>& GetIDXGISwapChainPresentDetour()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>& GetIDXGISwapChainPresentRefCount()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

std::unique_ptr<hadesmem::PatchDetour>& GetIDXGIFactoryCreateSwapChainDetour()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>& GetIDXGIFactoryCreateSwapChainRefCount()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

std::unique_ptr<hadesmem::PatchDetour>& GetCreateDXGIFactoryDetour()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>& GetCreateDXGIFactoryRefCount()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

std::pair<void*, SIZE_T>& GetDXGIModule() HADESMEM_DETAIL_NOEXCEPT
{
  static std::pair<void*, SIZE_T> module{nullptr, 0};
  return module;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnFrameCallbackDXGI>&
  GetOnFrameCallbacksDXGI()
{
  static hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnFrameCallbackDXGI>
    callbacks;
  return callbacks;
}

// Modified version of code from http://bit.ly/1iizOJR.
extern "C" HRESULT WINAPI
  IDXGISwapChainPresentDetour(IDXGISwapChain* swap_chain,
                              UINT sync_interval,
                              UINT flags) HADESMEM_DETAIL_NOEXCEPT
{
  hadesmem::detail::DetourRefCounter ref_count{
    GetIDXGISwapChainPresentRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A(
    "Args: [%p] [%u] [%u].", swap_chain, sync_interval, flags);

  try
  {
    auto& callbacks = GetOnFrameCallbacksDXGI();
    callbacks.Run(swap_chain);
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }

  auto& detour = GetIDXGISwapChainPresentDetour();
  auto const present =
    detour->GetTrampoline<decltype(&IDXGISwapChainPresentDetour)>();
  last_error_preserver.Revert();
  auto const ret = present(swap_chain, sync_interval, flags);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%ld].", ret);
  return ret;
}

extern "C" HRESULT WINAPI
  IDXGIFactoryCreateSwapChainDetour(IDXGIFactory* factory,
                                    IUnknown* device,
                                    DXGI_SWAP_CHAIN_DESC* desc,
                                    IDXGISwapChain** swap_chain)
  HADESMEM_DETAIL_NOEXCEPT
{
  hadesmem::detail::DetourRefCounter ref_count{
    GetIDXGIFactoryCreateSwapChainRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Args: [%p] [%p] [%p] [%p].", factory, device, desc, swap_chain);
  auto& detour = GetIDXGIFactoryCreateSwapChainDetour();
  auto const create_swap_chain =
    detour->GetTrampoline<decltype(&IDXGIFactoryCreateSwapChainDetour)>();
  last_error_preserver.Revert();
  auto const ret = create_swap_chain(factory, device, desc, swap_chain);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    if (swap_chain)
    {
      hadesmem::cerberus::DetourDXGISwapChain(*swap_chain);
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("Invalid swap chain out param pointer.");
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
  }

  return ret;
}

extern "C" HRESULT WINAPI CreateDXGIFactoryDetour(REFIID riid, void** factory)
  HADESMEM_DETAIL_NOEXCEPT
{
  hadesmem::detail::DetourRefCounter ref_count{GetCreateDXGIFactoryRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%p].", &riid, factory);
  auto& detour = GetCreateDXGIFactoryDetour();
  auto const create_dxgi_factory =
    detour->GetTrampoline<decltype(&CreateDXGIFactoryDetour)>();
  last_error_preserver.Revert();
  auto const ret = create_dxgi_factory(riid, factory);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    if (riid == __uuidof(IDXGIFactory))
    {
      HADESMEM_DETAIL_TRACE_A("Detouring IDXGIFactory.");

      hadesmem::cerberus::DetourDXGIFactory(
        static_cast<IDXGIFactory*>(*factory));
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("Unsupported GUID.");
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
  }

  return ret;
}
}

namespace hadesmem
{

namespace cerberus
{

void InitializeDXGI()
{
  InitializeSupportForModule(
    L"DXGI", &DetourDXGI, &UndetourDXGI, &GetDXGIModule);
}

void DetourDXGI(HMODULE base)
{
  HADESMEM_DETAIL_TRACE_A("Called.");

  auto& module = GetDXGIModule();
  if (module.first)
  {
    HADESMEM_DETAIL_TRACE_A("DXGI already detoured.");
    return;
  }

  if (!base)
  {
    base = ::GetModuleHandleW(L"DXGI");
  }

  if (!base)
  {
    HADESMEM_DETAIL_TRACE_A("Failed to find DXGI module.");
    return;
  }

  auto const& process = GetThisProcess();

  module =
    std::make_pair(base, hadesmem::detail::GetRegionAllocSize(process, base));

  DetourFunc(process,
             base,
             "CreateDXGIFactory",
             GetCreateDXGIFactoryDetour(),
             CreateDXGIFactoryDetour);
}

void UndetourDXGI(bool remove)
{
  auto& module = GetDXGIModule();
  if (!module.first)
  {
    HADESMEM_DETAIL_TRACE_A("DXGI not detoured.");
    return;
  }

  UndetourFunc(L"CreateDXGIFactory",
               GetCreateDXGIFactoryDetour(),
               GetCreateDXGIFactoryRefCount(),
               remove);
  UndetourFunc(L"IDXGIFactory::CreateSwapChain",
               GetIDXGIFactoryCreateSwapChainDetour(),
               GetIDXGIFactoryCreateSwapChainRefCount(),
               remove);
  UndetourFunc(L"IDXGISwapChain::Present",
               GetIDXGISwapChainPresentDetour(),
               GetIDXGISwapChainPresentRefCount(),
               remove);

  module = std::make_pair(nullptr, 0);
}

std::size_t RegisterOnFrameCallbackDXGI(
  std::function<OnFrameCallbackDXGI> const& callback)
{
  auto& callbacks = GetOnFrameCallbacksDXGI();
  return callbacks.Register(callback);
}

void UnregisterOnFrameCallbackDXGI(std::size_t id)
{
  auto& callbacks = GetOnFrameCallbacksDXGI();
  return callbacks.Unregister(id);
}

void DetourDXGISwapChain(IDXGISwapChain* swap_chain)
{
  try
  {
    auto const& process = GetThisProcess();
    DetourFunc(process,
               L"IDXGISwapChain::Present",
               swap_chain,
               8,
               GetIDXGISwapChainPresentDetour(),
               IDXGISwapChainPresentDetour);
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }
}

void DetourDXGIFactory(IDXGIFactory* dxgi_factory)
{
  try
  {
    auto const& process = GetThisProcess();
    DetourFunc(process,
               L"IDXGIFactory::CreateSwapChain",
               dxgi_factory,
               10,
               GetIDXGISwapChainPresentDetour(),
               IDXGISwapChainPresentDetour);
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }
}
}
}

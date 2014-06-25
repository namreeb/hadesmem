// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "dxgi.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <memory>
#include <mutex>
#include <string>

#include <windows.h>
#include <winnt.h>
#include <winternl.h>

#include <dxgi.h>
#include <dxgi1_3.h>

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

std::unique_ptr<hadesmem::PatchDetour>& GetIDXGISwapChain1Present1Detour()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>& GetIDXGIFactoryCreateSwapChainDetour()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetIDXGIFactory2CreateSwapChainForHwndDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetIDXGIFactory2CreateSwapChainForCoreWindowDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetIDXGIFactory2CreateSwapChainForCompositionDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>& GetCreateDXGIFactoryDetour()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>& GetCreateDXGIFactory1Detour()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>& GetCreateDXGIFactory2Detour()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
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

extern "C" HRESULT WINAPI
  IDXGISwapChainPresentDetour(IDXGISwapChain* swap_chain,
                              UINT sync_interval,
                              UINT flags) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetIDXGISwapChainPresentDetour();
  hadesmem::detail::DetourRefCounter ref_count{detour->GetRefCount()};
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

  auto const present =
    detour->GetTrampoline<decltype(&IDXGISwapChainPresentDetour)>();
  last_error_preserver.Revert();
  auto const ret = present(swap_chain, sync_interval, flags);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%ld].", ret);
  return ret;
}

extern "C" HRESULT WINAPI
  IDXGISwapChain1Present1Detour(IDXGISwapChain1* swap_chain,
                                UINT sync_interval,
                                UINT flags,
                                const DXGI_PRESENT_PARAMETERS* present_params)
  HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetIDXGISwapChain1Present1Detour();
  hadesmem::detail::DetourRefCounter ref_count{detour->GetRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p] [%u] [%u] [%p].",
                                       swap_chain,
                                       sync_interval,
                                       flags,
                                       present_params);

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

  auto const present_1 =
    detour->GetTrampoline<decltype(&IDXGISwapChain1Present1Detour)>();
  last_error_preserver.Revert();
  auto const ret = present_1(swap_chain, sync_interval, flags, present_params);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%ld].", ret);
  return ret;
}

void DetourDXGISwapChain(IDXGISwapChain* swap_chain)
{
  try
  {
    auto const& process = hadesmem::cerberus::GetThisProcess();
    hadesmem::cerberus::DetourFunc(process,
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

void DetourDXGISwapChain1(IDXGISwapChain1* swap_chain)
{
  try
  {
    auto const& process = hadesmem::cerberus::GetThisProcess();
    hadesmem::cerberus::DetourFunc(process,
                                   L"IDXGISwapChain1::Present1",
                                   swap_chain,
                                   22,
                                   GetIDXGISwapChain1Present1Detour(),
                                   IDXGISwapChain1Present1Detour);
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }

  DetourDXGISwapChain(swap_chain);
}

extern "C" HRESULT WINAPI
  IDXGIFactoryCreateSwapChainDetour(IDXGIFactory* factory,
                                    IUnknown* device,
                                    DXGI_SWAP_CHAIN_DESC* desc,
                                    IDXGISwapChain** swap_chain)
  HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetIDXGIFactoryCreateSwapChainDetour();
  hadesmem::detail::DetourRefCounter ref_count{detour->GetRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Args: [%p] [%p] [%p] [%p].", factory, device, desc, swap_chain);
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
      hadesmem::cerberus::DetourDXGISwapChainByRevision(*swap_chain, 0);
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

extern "C" HRESULT WINAPI IDXGIFactory2CreateSwapChainForHwndDetour(
  IDXGIFactory2* factory,
  IUnknown* device,
  HWND hwnd,
  const DXGI_SWAP_CHAIN_DESC1* desc,
  const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreen_desc,
  IDXGIOutput* restrict_to_output,
  IDXGISwapChain1** swap_chain) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetIDXGIFactory2CreateSwapChainForHwndDetour();
  hadesmem::detail::DetourRefCounter ref_count{detour->GetRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%p] [%p] [%p] [%p] [%p] [%p].",
                                 factory,
                                 device,
                                 hwnd,
                                 desc,
                                 fullscreen_desc,
                                 restrict_to_output,
                                 swap_chain);
  auto const create_swap_chain_for_hwnd =
    detour
      ->GetTrampoline<decltype(&IDXGIFactory2CreateSwapChainForHwndDetour)>();
  last_error_preserver.Revert();
  auto const ret = create_swap_chain_for_hwnd(factory,
                                              device,
                                              hwnd,
                                              desc,
                                              fullscreen_desc,
                                              restrict_to_output,
                                              swap_chain);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    if (swap_chain)
    {
      hadesmem::cerberus::DetourDXGISwapChainByRevision(*swap_chain, 1);
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

extern "C" HRESULT WINAPI IDXGIFactory2CreateSwapChainForCoreWindowDetour(
  IDXGIFactory2* factory,
  IUnknown* device,
  IUnknown* window,
  const DXGI_SWAP_CHAIN_DESC1* desc,
  IDXGIOutput* restrict_to_output,
  IDXGISwapChain1** swap_chain) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetIDXGIFactory2CreateSwapChainForCoreWindowDetour();
  hadesmem::detail::DetourRefCounter ref_count{detour->GetRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%p] [%p] [%p] [%p] [%p].",
                                 factory,
                                 device,
                                 window,
                                 desc,
                                 restrict_to_output,
                                 swap_chain);
  auto const create_swap_chain_for_core_window = detour->GetTrampoline<
    decltype(&IDXGIFactory2CreateSwapChainForCoreWindowDetour)>();
  last_error_preserver.Revert();
  auto const ret = create_swap_chain_for_core_window(
    factory, device, window, desc, restrict_to_output, swap_chain);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    if (swap_chain)
    {
      hadesmem::cerberus::DetourDXGISwapChainByRevision(*swap_chain, 1);
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

extern "C" HRESULT WINAPI IDXGIFactory2CreateSwapChainForCompositionDetour(
  IDXGIFactory2* factory,
  IUnknown* device,
  const DXGI_SWAP_CHAIN_DESC1* desc,
  IDXGIOutput* restrict_to_output,
  IDXGISwapChain1** swap_chain) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetIDXGIFactory2CreateSwapChainForCompositionDetour();
  hadesmem::detail::DetourRefCounter ref_count{detour->GetRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%p] [%p] [%p] [%p].",
                                 factory,
                                 device,
                                 desc,
                                 restrict_to_output,
                                 swap_chain);
  auto const create_swap_chain_for_composition = detour->GetTrampoline<
    decltype(&IDXGIFactory2CreateSwapChainForCompositionDetour)>();
  last_error_preserver.Revert();
  auto const ret = create_swap_chain_for_composition(
    factory, device, desc, restrict_to_output, swap_chain);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    if (swap_chain)
    {
      hadesmem::cerberus::DetourDXGISwapChainByRevision(*swap_chain, 1);
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

void DetourDXGIFactory(IDXGIFactory* dxgi_factory)
{
  try
  {
    auto const& process = hadesmem::cerberus::GetThisProcess();
    hadesmem::cerberus::DetourFunc(process,
                                   L"IDXGIFactory::CreateSwapChain",
                                   dxgi_factory,
                                   10,
                                   GetIDXGIFactoryCreateSwapChainDetour(),
                                   IDXGIFactoryCreateSwapChainDetour);
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }
}

void DetourDXGIFactory2(IDXGIFactory2* dxgi_factory)
{
  try
  {
    auto const& process = hadesmem::cerberus::GetThisProcess();
    hadesmem::cerberus::DetourFunc(
      process,
      L"IDXGIFactory2::CreateSwapChainForHwnd",
      dxgi_factory,
      15,
      GetIDXGIFactory2CreateSwapChainForHwndDetour(),
      IDXGIFactory2CreateSwapChainForHwndDetour);
    hadesmem::cerberus::DetourFunc(
      process,
      L"IDXGIFactory2::CreateSwapChainForCoreWindow",
      dxgi_factory,
      16,
      GetIDXGIFactory2CreateSwapChainForCoreWindowDetour(),
      IDXGIFactory2CreateSwapChainForCoreWindowDetour);
    hadesmem::cerberus::DetourFunc(
      process,
      L"IDXGIFactory2::CreateSwapChainForComposition",
      dxgi_factory,
      24,
      GetIDXGIFactory2CreateSwapChainForCompositionDetour(),
      IDXGIFactory2CreateSwapChainForCompositionDetour);
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }

  DetourDXGIFactory(dxgi_factory);
}

void DetourDXGIFactoryByGuid(REFIID riid, void** factory)
{

  if (riid == __uuidof(IDXGIFactory) || riid == __uuidof(IDXGIFactory1))
  {
    std::uint32_t const revision = riid == __uuidof(IDXGIFactory) ? 0 : 1;
    hadesmem::cerberus::DetourDXGIFactoryByRevision(static_cast<IDXGIFactory*>(*factory), revision);
  }
  else if (riid == __uuidof(IDXGIFactory2))
  {
    hadesmem::cerberus::DetourDXGIFactoryByRevision(static_cast<IDXGIFactory2*>(*factory), 2);
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Unsupported GUID.");
  }
}

extern "C" HRESULT WINAPI CreateDXGIFactoryDetour(REFIID riid, void** factory)
  HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetCreateDXGIFactoryDetour();
  hadesmem::detail::DetourRefCounter ref_count{detour->GetRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%p].", &riid, factory);
  auto const create_dxgi_factory =
    detour->GetTrampoline<decltype(&CreateDXGIFactoryDetour)>();
  last_error_preserver.Revert();
  auto const ret = create_dxgi_factory(riid, factory);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    DetourDXGIFactoryByGuid(riid, factory);
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
  }

  return ret;
}

extern "C" HRESULT WINAPI CreateDXGIFactory1Detour(REFIID riid, void** factory)
  HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetCreateDXGIFactory1Detour();
  hadesmem::detail::DetourRefCounter ref_count{detour->GetRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%p].", &riid, factory);
  auto const create_dxgi_factory_1 =
    detour->GetTrampoline<decltype(&CreateDXGIFactory1Detour)>();
  last_error_preserver.Revert();
  auto const ret = create_dxgi_factory_1(riid, factory);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    DetourDXGIFactoryByGuid(riid, factory);
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
  }

  return ret;
}

extern "C" HRESULT WINAPI
  CreateDXGIFactory2Detour(UINT flags, REFIID riid, void** factory)
  HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetCreateDXGIFactory2Detour();
  hadesmem::detail::DetourRefCounter ref_count{detour->GetRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%p].", &riid, factory);
  auto const create_dxgi_factory_2 =
    detour->GetTrampoline<decltype(&CreateDXGIFactory2Detour)>();
  last_error_preserver.Revert();
  auto const ret = create_dxgi_factory_2(flags, riid, factory);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    DetourDXGIFactoryByGuid(riid, factory);
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
  auto const& process = GetThisProcess();
  auto& module = GetDXGIModule();
  if (CommonDetourModule(process, L"DXGI", base, module))
  {
    DetourFunc(process,
               base,
               "CreateDXGIFactory",
               GetCreateDXGIFactoryDetour(),
               CreateDXGIFactoryDetour);
    DetourFunc(process,
               base,
               "CreateDXGIFactory1",
               GetCreateDXGIFactory1Detour(),
               CreateDXGIFactory1Detour);
    DetourFunc(process,
               base,
               "CreateDXGIFactory2",
               GetCreateDXGIFactory2Detour(),
               CreateDXGIFactory2Detour);
  }
}

void UndetourDXGI(bool remove)
{
  auto& module = GetDXGIModule();
  if (CommonUndetourModule(L"DXGI", module))
  {
    UndetourFunc(L"CreateDXGIFactory", GetCreateDXGIFactoryDetour(), remove);
    UndetourFunc(L"CreateDXGIFactory1", GetCreateDXGIFactory1Detour(), remove);
    UndetourFunc(L"CreateDXGIFactory2", GetCreateDXGIFactory2Detour(), remove);
    UndetourFunc(L"IDXGIFactory::CreateSwapChain",
                 GetIDXGIFactoryCreateSwapChainDetour(),
                 remove);
    UndetourFunc(L"IDXGIFactory::CreateSwapChainForHwnd",
                 GetIDXGIFactory2CreateSwapChainForHwndDetour(),
                 remove);
    UndetourFunc(L"IDXGIFactory::CreateSwapChainForCoreWindow",
                 GetIDXGIFactory2CreateSwapChainForCoreWindowDetour(),
                 remove);
    UndetourFunc(L"IDXGIFactory::CreateSwapChainForComposition",
                 GetIDXGIFactory2CreateSwapChainForCompositionDetour(),
                 remove);
    UndetourFunc(
      L"IDXGISwapChain::Present", GetIDXGISwapChainPresentDetour(), remove);
    UndetourFunc(
      L"IDXGISwapChain1::Present1", GetIDXGISwapChain1Present1Detour(), remove);

    module = std::make_pair(nullptr, 0);
  }
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

void DetourDXGISwapChainByRevision(IDXGISwapChain* swap_chain,
                                   std::uint32_t revision)
{
  if (revision == 0)
  {
    HADESMEM_DETAIL_TRACE_A("Detouring IDXGISwapChain.");
    DetourDXGISwapChain(swap_chain);
  }
  else if (revision == 1)
  {
    HADESMEM_DETAIL_TRACE_A("Detouring IDXGISwapChain1.");
    DetourDXGISwapChain1(static_cast<IDXGISwapChain1*>(swap_chain));
  }
  else
  {
    HADESMEM_DETAIL_ASSERT(false);
    HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error{} << hadesmem::ErrorString{
                                      "Unknown DXGI swap chain revision."});
  }
}

void DetourDXGIFactoryByRevision(IDXGIFactory* dxgi_factory,
                                 std::uint32_t revision)
{
  if (revision < 2)
  {
    HADESMEM_DETAIL_TRACE_A("Detouring IDXGIFactory/IDXGIFactory1.");
    DetourDXGIFactory(dxgi_factory);
  }
  else if (revision == 2)
  {
    HADESMEM_DETAIL_TRACE_A("Detouring IDXGIFactory/IDXGIFactory2.");
    DetourDXGIFactory2(static_cast<IDXGIFactory2*>(dxgi_factory));
  }
  else
  {
    HADESMEM_DETAIL_ASSERT(false);
    HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error{} << hadesmem::ErrorString{
                                      "Unknown DXGI factory revision."});
  }
}
}
}

// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "dxgi.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <memory>
#include <string>

#include <windows.h>
#include <winnt.h>
#include <winternl.h>

#include <dxgi1_2.h>
#include <dxgi.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/detour_ref_counter.hpp>
#include <hadesmem/detail/last_error_preserver.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/detail/winternl.hpp>
#include <hadesmem/find_procedure.hpp>
#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/region.hpp>

#include "callbacks.hpp"
#include "dxgi_helpers.hpp"
#include "helpers.hpp"
#include "main.hpp"
#include "module.hpp"

namespace
{
hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnFrameDXGICallback>&
  GetOnFrameDXGICallbacks()
{
  static hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnFrameDXGICallback>
    callbacks;
  return callbacks;
}

class DXGIImpl : public hadesmem::cerberus::DXGIInterface
{
public:
  virtual std::size_t RegisterOnFrame(
    std::function<hadesmem::cerberus::OnFrameDXGICallback> const& callback)
    final
  {
    auto& callbacks = GetOnFrameDXGICallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnFrame(std::size_t id) final
  {
    auto& callbacks = GetOnFrameDXGICallbacks();
    return callbacks.Unregister(id);
  }
};

std::unique_ptr<hadesmem::PatchDetour>&
  GetIDXGISwapChainPresentDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetIDXGISwapChain1Present1Detour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetIDXGIFactoryCreateSwapChainDetour() HADESMEM_DETAIL_NOEXCEPT
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

std::unique_ptr<hadesmem::PatchDetour>&
  GetCreateDXGIFactoryDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetCreateDXGIFactory1Detour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetCreateDXGIFactory2Detour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::pair<void*, SIZE_T>& GetDXGIModule() HADESMEM_DETAIL_NOEXCEPT
{
  static std::pair<void*, SIZE_T> module{nullptr, 0};
  return module;
}

bool& GetInPresentHook() HADESMEM_DETAIL_NOEXCEPT
{
#if defined(HADESMEM_GCC) || defined(HADESMEM_CLANG)
  static thread_local bool in_hook = false;
#elif defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
  static __declspec(thread) bool in_hook = false;
#else
#error "[HadesMem] Unsupported compiler."
#endif
  return in_hook;
}

extern "C" HRESULT WINAPI
  IDXGISwapChainPresentDetour(IDXGISwapChain* swap_chain,
                              UINT sync_interval,
                              UINT flags) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetIDXGISwapChainPresentDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A(
    "Args: [%p] [%u] [%u].", swap_chain, sync_interval, flags);

  auto& in_hook = GetInPresentHook();
  if (in_hook)
  {
    HADESMEM_DETAIL_TRACE_NOISY_A("Detected already handled frame. Skipping.");
  }
  else
  {
    auto& callbacks = GetOnFrameDXGICallbacks();
    callbacks.Run(swap_chain);
  }

  auto const present =
    detour->GetTrampoline<decltype(&IDXGISwapChainPresentDetour)>();
  last_error_preserver.Revert();
  auto const ret = present(swap_chain, sync_interval, flags);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%ld].", ret);
  return ret;
}

extern "C" HRESULT WINAPI IDXGISwapChain1Present1Detour(
  IDXGISwapChain* swap_chain,
  UINT sync_interval,
  UINT flags,
  const DXGI_PRESENT_PARAMETERS* present_params) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetIDXGISwapChain1Present1Detour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A(
    "Args: [%p] [%u] [%u].", swap_chain, sync_interval, flags);

  auto& in_hook = GetInPresentHook();
  in_hook = true;
  auto reset_in_hook_fn = [&]()
  {
    in_hook = false;
  };
  auto const ensure_reset_in_hook =
    hadesmem::detail::MakeScopeWarden(reset_in_hook_fn);

  auto& callbacks = GetOnFrameDXGICallbacks();
  callbacks.Run(swap_chain);

  auto const present1 =
    detour->GetTrampoline<decltype(&IDXGISwapChain1Present1Detour)>();
  last_error_preserver.Revert();
  auto const ret = present1(swap_chain, sync_interval, flags, present_params);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%ld].", ret);
  return ret;
}

extern "C" HRESULT WINAPI IDXGIFactoryCreateSwapChainDetour(
  IDXGIFactory* factory,
  IUnknown* device,
  DXGI_SWAP_CHAIN_DESC* desc,
  IDXGISwapChain** swap_chain) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetIDXGIFactoryCreateSwapChainDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
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
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
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

extern "C" HRESULT WINAPI IDXGIFactory2CreateSwapChainForCoreWindowDetour(
  IDXGIFactory2* factory,
  IUnknown* device,
  IUnknown* window,
  const DXGI_SWAP_CHAIN_DESC1* desc,
  IDXGIOutput* restrict_to_output,
  IDXGISwapChain1** swap_chain) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetIDXGIFactory2CreateSwapChainForCoreWindowDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%p] [%p] [%p] [%p] [%p].",
                                 factory,
                                 device,
                                 window,
                                 desc,
                                 restrict_to_output,
                                 swap_chain);
  auto const create_swap_chain_for_core_window = detour->GetTrampoline<decltype(
    &IDXGIFactory2CreateSwapChainForCoreWindowDetour)>();
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

extern "C" HRESULT WINAPI IDXGIFactory2CreateSwapChainForCompositionDetour(
  IDXGIFactory2* factory,
  IUnknown* device,
  const DXGI_SWAP_CHAIN_DESC1* desc,
  IDXGIOutput* restrict_to_output,
  IDXGISwapChain1** swap_chain) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetIDXGIFactory2CreateSwapChainForCompositionDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%p] [%p] [%p] [%p].",
                                 factory,
                                 device,
                                 desc,
                                 restrict_to_output,
                                 swap_chain);
  auto const create_swap_chain_for_composition = detour->GetTrampoline<decltype(
    &IDXGIFactory2CreateSwapChainForCompositionDetour)>();
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

void DetourDXGIFactoryByGuid(REFIID riid, void** factory)
{
  if (riid == __uuidof(IDXGIFactory))
  {
    hadesmem::cerberus::DetourDXGIFactory(static_cast<IDXGIFactory*>(*factory));
  }
  else if (riid == __uuidof(IDXGIFactory1))
  {
    hadesmem::cerberus::DetourDXGIFactory(
      static_cast<IDXGIFactory1*>(*factory));
  }
  else if (riid == __uuidof(IDXGIFactory2))
  {
    hadesmem::cerberus::DetourDXGIFactory(
      static_cast<IDXGIFactory2*>(*factory));
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Unsupported GUID.");
  }
}

extern "C" HRESULT WINAPI
  CreateDXGIFactoryDetour(REFIID riid, void** factory) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetCreateDXGIFactoryDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
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

extern "C" HRESULT WINAPI
  CreateDXGIFactory1Detour(REFIID riid, void** factory) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetCreateDXGIFactory1Detour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
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
  CreateDXGIFactory2Detour(UINT flags,
                           REFIID riid,
                           void** factory) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetCreateDXGIFactory2Detour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
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
DXGIInterface& GetDXGIInterface() HADESMEM_DETAIL_NOEXCEPT
{
  static DXGIImpl dxgi_impl;
  return dxgi_impl;
}

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
    UndetourFunc(L"IDXGIFactory2::CreateSwapChainForHwnd",
                 GetIDXGIFactory2CreateSwapChainForHwndDetour(),
                 remove);
    UndetourFunc(L"IDXGIFactory2::CreateSwapChainForCoreWindow",
                 GetIDXGIFactory2CreateSwapChainForCoreWindowDetour(),
                 remove);
    UndetourFunc(L"IDXGIFactory2::CreateSwapChainForComposition",
                 GetIDXGIFactory2CreateSwapChainForCompositionDetour(),
                 remove);
    UndetourFunc(
      L"IDXGISwapChain::Present", GetIDXGISwapChainPresentDetour(), remove);
    UndetourFunc(
      L"IDXGISwapChain1::Present1", GetIDXGISwapChain1Present1Detour(), remove);

    module = std::make_pair(nullptr, 0);
  }
}

void DetourDXGISwapChain(IDXGISwapChain* swap_chain)
{
  HADESMEM_DETAIL_TRACE_A("Detouring IDXGISwapChain.");

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

void DetourDXGISwapChain(IDXGISwapChain1* swap_chain)
{
  HADESMEM_DETAIL_TRACE_A("Detouring IDXGISwapChain1.");

  try
  {
    auto const& process = GetThisProcess();
    DetourFunc(process,
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

  DetourDXGISwapChain(static_cast<IDXGISwapChain*>(swap_chain));
}

void DetourDXGIFactory(IDXGIFactory* dxgi_factory)
{
  HADESMEM_DETAIL_TRACE_A("Detouring IDXGIFactory.");

  try
  {
    auto const& process = GetThisProcess();
    DetourFunc(process,
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

void DetourDXGIFactory(IDXGIFactory2* dxgi_factory)
{
  HADESMEM_DETAIL_TRACE_A("Detouring IDXGIFactory2.");

  try
  {
    auto const& process = GetThisProcess();

    DetourFunc(process,
               L"IDXGIFactory2::CreateSwapChainForHwnd",
               dxgi_factory,
               15,
               GetIDXGIFactory2CreateSwapChainForHwndDetour(),
               IDXGIFactory2CreateSwapChainForHwndDetour);

    DetourFunc(process,
               L"IDXGIFactory2::CreateSwapChainForCoreWindow",
               dxgi_factory,
               16,
               GetIDXGIFactory2CreateSwapChainForCoreWindowDetour(),
               IDXGIFactory2CreateSwapChainForCoreWindowDetour);

    DetourFunc(process,
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

  DetourDXGIFactory(static_cast<IDXGIFactory1*>(dxgi_factory));
}

void DetourDXGIFactoryFromDevice(IUnknown* device)
{
  HADESMEM_DETAIL_TRACE_A("Detouring factory from device.");

  auto const factory_wrapper = GetDXGIFactoryFromDevice(device);
  if (auto const factory = factory_wrapper.GetFactory())
  {
    switch (factory_wrapper.GetFactoryRevision())
    {
    case 0:
      HADESMEM_DETAIL_TRACE_A("Detected IDXGIFactory.");
      DetourDXGIFactory(factory);
      break;
    case 1:
      HADESMEM_DETAIL_TRACE_A("Detected IDXGIFactory1.");
      DetourDXGIFactory(static_cast<IDXGIFactory1*>(factory));
      break;
    case 2:
      HADESMEM_DETAIL_TRACE_A("Detected IDXGIFactory2.");
      DetourDXGIFactory(static_cast<IDXGIFactory2*>(factory));
      break;
    default:
      HADESMEM_DETAIL_TRACE_A("Unknown IDXGIFactory interface.");
      HADESMEM_DETAIL_ASSERT(false);
      break;
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Invalid factory pointer.");
  }
}
}
}

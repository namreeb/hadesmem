/*
This file is part of HadesMem.
Copyright (C) 2011 Joshua Boyce (a.k.a. RaptorFactor).
<http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

HadesMem is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

HadesMem is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with HadesMem.  If not, see <http://www.gnu.org/licenses/>.
*/

// C++ Standard Library
#include <iostream>

// Boost
#include <boost/format.hpp>
#include <boost/thread.hpp>

// Hades
#include "Hooker.hpp"
#include "HadesMemory/Memory.hpp"

namespace Hades
{
  namespace D3D11
  {
    // Todo: Ensure hook removal on module unload
    // Todo: Deferred hooking via a LoadLibrary hook
    // Todo: Fail gracefully if particular export/interface not supported
    
      // Kernel instance
    Kernel::Kernel* D3D11Hooker::m_pKernel = nullptr;
  
    // Renderer
    std::shared_ptr<GUI::D3D11Renderer> D3D11Hooker::m_pRenderer;
    
    // d3d11.dll!D3D11CreateDeviceAndSwapChain hook
    std::shared_ptr<Hades::Memory::PatchDetour> D3D11Hooker::
      m_pD3D11CreateDeviceAndSwapChainHk;
    
    // dxgi.dll!CreateDXGIFactory hook
    std::shared_ptr<Hades::Memory::PatchDetour> D3D11Hooker::
      m_pCreateDXGIFactoryHk;
    
    // dxgi.dll!CreateDXGIFactory1 hook
    std::shared_ptr<Hades::Memory::PatchDetour> D3D11Hooker::
      m_pCreateDXGIFactory1Hk;
        
    // dxgi.dll!IDXGIFactory::CreateSwapChain hook
    std::shared_ptr<Hades::Memory::PatchDetour> D3D11Hooker::
      m_pCreateSwapChainHk;
    
    // dxgi.dll!IDXGISwapChain::Present hook
    std::shared_ptr<Hades::Memory::PatchDetour> D3D11Hooker::m_pPresentHk;
    
    // Initialize D3D11 hooker
    void D3D11Hooker::Initialize(Kernel::Kernel& MyKernel)
    {
      m_pKernel = &MyKernel;
    }
    
    // Hook D3D11
    void D3D11Hooker::Hook()
    {
      // Check if already hooked
      if (m_pD3D11CreateDeviceAndSwapChainHk || m_pCreateDXGIFactoryHk || 
        m_pCreateDXGIFactory1Hk || m_pCreateSwapChainHk || m_pPresentHk)
      {
        if (m_pD3D11CreateDeviceAndSwapChainHk)
        {
          m_pD3D11CreateDeviceAndSwapChainHk->Apply();
        }
        
        if (m_pCreateDXGIFactoryHk)
        {
          m_pCreateDXGIFactoryHk->Apply();
        }
        
        if (m_pCreateDXGIFactory1Hk)
        {
          m_pCreateDXGIFactory1Hk->Apply();
        }
        
        if (m_pCreateSwapChainHk)
        {
          m_pCreateSwapChainHk->Apply();
        }
        
        if (m_pPresentHk)
        {
          m_pPresentHk->Apply();
        }
        
        return;
      }
      
      // Get d3d11.dll module handle
      HMODULE D3D11Mod = LoadLibrary(L"d3d11.dll");
      if (!D3D11Mod)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("D3D11Hooker::Hook") << 
          ErrorString("Could not load d3d11.dll") << 
          ErrorCode(LastError));
      }
      
      // Find d3d11.dll!D3D11CreateDeviceAndSwapChain
      FARPROC pD3D11CreateDeviceAndSwapChain = GetProcAddress(D3D11Mod, 
        "D3D11CreateDeviceAndSwapChain");
      if (!pD3D11CreateDeviceAndSwapChain)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("D3D11Hooker::Hook") << 
          ErrorString("Could not find d3d11.dll!"
          "D3D11CreateDeviceAndSwapChain") << 
          ErrorCode(LastError));
      }
      
      // Debug output
      std::wcout << boost::wformat(L"D3D11Hooker::Hook: Hooking "
        L"d3d11.dll!D3D11CreateDeviceAndSwapChain (%p).") 
        %pD3D11CreateDeviceAndSwapChain << std::endl;
          
      // Create memory manager
      Memory::MemoryMgr const MyMemory(GetCurrentProcessId());
          
      // Hook d3d11.dll!D3D11CreateDeviceAndSwapChain
      m_pD3D11CreateDeviceAndSwapChainHk.reset(new Memory::PatchDetour(
        MyMemory, pD3D11CreateDeviceAndSwapChain, 
        &D3D11CreateDeviceAndSwapChain_Hook));
      m_pD3D11CreateDeviceAndSwapChainHk->Apply();
      
      // Get dxgi.dll module handle
      HMODULE DXGIMod = LoadLibrary(L"dxgi.dll");
      if (!DXGIMod)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("D3D11Hooker::Hook") << 
          ErrorString("Could not load dxgi.dll") << 
          ErrorCode(LastError));
      }
      
      // Find dxgi.dll!CreateDXGIFactory
      FARPROC pCreateDXGIFactory = GetProcAddress(DXGIMod, 
        "CreateDXGIFactory");
      if (!pCreateDXGIFactory)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("D3D11Hooker::Hook") << 
          ErrorString("Could not find dxgi.dll!CreateDXGIFactory") << 
          ErrorCode(LastError));
      }
      
      // Debug output
      std::wcout << boost::wformat(L"D3D11Hooker::Hook: Hooking "
        L"dxgi.dll!CreateDXGIFactory (%p).") %pCreateDXGIFactory 
        << std::endl;
          
      // Hook dxgi.dll!CreateDXGIFactory
      m_pCreateDXGIFactoryHk.reset(new Memory::PatchDetour(MyMemory, 
        pCreateDXGIFactory, &CreateDXGIFactory_Hook));
      m_pCreateDXGIFactoryHk->Apply();
      
      // Find dxgi.dll!CreateDXGIFactory1
      FARPROC pCreateDXGIFactory1 = GetProcAddress(DXGIMod, 
        "CreateDXGIFactory1");
      if (!pCreateDXGIFactory1)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("D3D11Hooker::Hook") << 
          ErrorString("Could not find dxgi.dll!CreateDXGIFactory1") << 
          ErrorCode(LastError));
      }
      
      // Debug output
      std::wcout << boost::wformat(L"D3D11Hooker::Hook: Hooking "
        L"dxgi.dll!CreateDXGIFactory1 (%p).") %pCreateDXGIFactory1 
        << std::endl;
          
      // Hook dxgi.dll!CreateDXGIFactory1
      m_pCreateDXGIFactory1Hk.reset(new Memory::PatchDetour(MyMemory, 
        pCreateDXGIFactory1, &CreateDXGIFactory1_Hook));
      m_pCreateDXGIFactory1Hk->Apply();
    }
    
    // Unhook D3D11
    void D3D11Hooker::Unhook()
    {
      if (m_pD3D11CreateDeviceAndSwapChainHk)
      {
        m_pD3D11CreateDeviceAndSwapChainHk->Remove();
      }
        
      if (m_pCreateDXGIFactoryHk)
      {
        m_pCreateDXGIFactoryHk->Remove();
      }
      
      if (m_pCreateDXGIFactory1Hk)
      {
        m_pCreateDXGIFactory1Hk->Remove();
      }
      
      if (m_pCreateSwapChainHk)
      {
        m_pCreateSwapChainHk->Remove();
      }
      
      if (m_pPresentHk)
      {
        m_pPresentHk->Remove();
      }
    }
    
    // d3d11.dll!D3D11CreateDeviceAndSwapChain hook implementation
    HRESULT WINAPI D3D11Hooker::D3D11CreateDeviceAndSwapChain_Hook(
      IDXGIAdapter *pAdapter,
      D3D_DRIVER_TYPE DriverType,
      HMODULE Software,
      UINT Flags,
      const D3D_FEATURE_LEVEL *pFeatureLevels,
      UINT FeatureLevels,
      UINT SDKVersion,
      const DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
      IDXGISwapChain **ppSwapChain,
      ID3D11Device **ppDevice,
      D3D_FEATURE_LEVEL *pFeatureLevel,
      ID3D11DeviceContext **ppImmediateContext)
    {
      // Function is not thread-safe
      static boost::mutex MyMutex;
      boost::lock_guard<boost::mutex> MyLock(MyMutex);
        
      try
      {
        // Call trampoline
        typedef HRESULT (WINAPI* tD3D11CreateDeviceAndSwapChain)(
          IDXGIAdapter *pAdapter,
          D3D_DRIVER_TYPE DriverType,
          HMODULE Software,
          UINT Flags,
          const D3D_FEATURE_LEVEL *pFeatureLevels,
          UINT FeatureLevels,
          UINT SDKVersion,
          const DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
          IDXGISwapChain **ppSwapChain,
          ID3D11Device **ppDevice,
          D3D_FEATURE_LEVEL *pFeatureLevel,
          ID3D11DeviceContext **ppImmediateContext);
        auto pD3D11CreateDeviceAndSwapChain = 
          reinterpret_cast<tD3D11CreateDeviceAndSwapChain>(
          m_pD3D11CreateDeviceAndSwapChainHk->GetTrampoline());
        HRESULT Result = pD3D11CreateDeviceAndSwapChain(
          pAdapter, 
          DriverType, 
          Software, 
          Flags, 
          pFeatureLevels, 
          FeatureLevels, 
          SDKVersion, 
          pSwapChainDesc, 
          ppSwapChain, 
          ppDevice, 
          pFeatureLevel, 
          ppImmediateContext);
        
        // Debug output
        std::wcout << boost::wformat(L"D3D11Hooker::"
          L"D3D11CreateDeviceAndSwapChain_Hook: pAdapter = %p, "
          L"DriverType = %u, Software = %p, Flags = %u, pFeatureLevels = %p, "
          L"FeatureLevels = %u, SDKVersion = %u, pSwapChainDesc = %p, "
          L"ppSwapChain = %p, ppDevice = %p, pFeatureLevel = %p, "
          L"ppImmediateContext = %p. Return = %u.") 
          %pAdapter %DriverType %Software %Flags %pFeatureLevels 
          %FeatureLevels %SDKVersion %pSwapChainDesc %ppSwapChain %ppDevice 
          %pFeatureLevel %ppImmediateContext %Result << std::endl;
            
        // Create renderer if call successful
        if (SUCCEEDED(Result))
        {
          std::wcout << "D3D11Hooker::D3D11CreateDeviceAndSwapChain_Hook: "
            "Creating D3D11Renderer." << std::endl;
            
          m_pRenderer.reset(new GUI::D3D11Renderer(*ppDevice, 
            *ppImmediateContext));
        }
        
        // Return result from trampoline
        return Result;
      }
      catch (std::exception const& e)
      {
        // Debug output
        std::cout << boost::format("D3D11Hooker::"
          "D3D11CreateDeviceAndSwapChain_Hook: Error! %s.") %e.what() 
          << std::endl;
            
        // Failure
        return E_FAIL;
      }
    }
    
    // dxgi.dll!CreateDXGIFactory hook implementation
    HRESULT WINAPI D3D11Hooker::CreateDXGIFactory_Hook(
      REFIID riid,
      void **ppFactory)
    {
      // Function is not thread-safe
      static boost::mutex MyMutex;
      boost::lock_guard<boost::mutex> MyLock(MyMutex);
        
      try
      {
        // Call trampoline
        typedef HRESULT (WINAPI* tCreateDXGIFactory)(
          REFIID riid,
          void **ppFactory);
        auto pCreateDXGIFactory = reinterpret_cast<tCreateDXGIFactory>(
          m_pCreateDXGIFactoryHk->GetTrampoline());
        HRESULT Result = pCreateDXGIFactory(riid, ppFactory);
          
        // Debug output
        std::wcout << boost::wformat(L"D3D11Hooker::CreateDXGIFactory_Hook: "
          L"riid = ?, ppFactory = %p. Return = %u.") 
          /*%riid*/ %ppFactory %Result << std::endl;
            
        // Hook if call successful
        if (SUCCEEDED(Result) && !m_pCreateSwapChainHk)
        {
          // Get pointer to dxgi.dll!IDXGIFactory::CreateSwapChain
          PVOID* pFactoryVMT = *reinterpret_cast<PVOID**>(*ppFactory);
          PVOID pCreateSwapChain = pFactoryVMT[10];
          
          // Debug output
          std::wcout << boost::wformat(L"D3D11Hooker::CreateDXGIFactory_Hook: "
            L"Hooking dxgi.dll!IDXGIFactory::CreateSwapChain (%p).") 
            %pCreateSwapChain << std::endl;

          // Hook dxgi.dll!IDXGIFactory::CreateSwapChain
          Memory::MemoryMgr const MyMemory(GetCurrentProcessId());
          m_pCreateSwapChainHk.reset(new Hades::Memory::PatchDetour(MyMemory, 
            pCreateSwapChain, &CreateSwapChain_Hook));
          m_pCreateSwapChainHk->Apply();
        }
        
        // Return result from trampoline
        return Result;
      }
      catch (std::exception const& e)
      {
        // Debug output
        std::cout << boost::format("D3D11Hooker::CreateDXGIFactory_Hook: "
          "Error! %s.") %e.what() << std::endl;
            
        // Failure
        return E_FAIL;
      }
    }
    
    // dxgi.dll!CreateDXGIFactory1 hook implementation
    HRESULT WINAPI D3D11Hooker::CreateDXGIFactory1_Hook(
      REFIID riid, 
      void **ppFactory)
    {
      // Function is not thread-safe
      static boost::mutex MyMutex;
      boost::lock_guard<boost::mutex> MyLock(MyMutex);
        
      try
      {
        // Call trampoline
        typedef HRESULT (WINAPI* tCreateDXGIFactory1)(
          REFIID riid,
          void **ppFactory);
        auto pCreateDXGIFactory1 = reinterpret_cast<tCreateDXGIFactory1>(
          m_pCreateDXGIFactory1Hk->GetTrampoline());
        HRESULT Result = pCreateDXGIFactory1(riid, ppFactory);
            
        // Debug output
        std::wcout << boost::wformat(L"D3D11Hooker::CreateDXGIFactory1_Hook: "
          L"riid = ?, ppFactory = %p. Return = %u.") 
          /*%riid*/ %ppFactory %Result << std::endl;
            
        // Hook if call successful
        if (SUCCEEDED(Result) && !m_pCreateSwapChainHk)
        {
          // Get pointer to dxgi.dll!IDXGIFactory::CreateSwapChain
          PVOID* pFactoryVMT = *reinterpret_cast<PVOID**>(*ppFactory);
          PVOID pCreateSwapChain = pFactoryVMT[10];
          
          // Debug output
          std::wcout << boost::wformat(L"D3D11Hooker::CreateDXGIFactory1_Hook: "
            L"Hooking dxgi.dll!IDXGIFactory::CreateSwapChain (%p).") 
            %pCreateSwapChain << std::endl;

          // Hook dxgi.dll!IDXGIFactory::CreateSwapChain
          Memory::MemoryMgr const MyMemory(GetCurrentProcessId());
          m_pCreateSwapChainHk.reset(new Hades::Memory::PatchDetour(MyMemory, 
            pCreateSwapChain, &CreateSwapChain_Hook));
          m_pCreateSwapChainHk->Apply();
        }
        
        // Return result from trampoline
        return Result;
      }
      catch (std::exception const& e)
      {
        // Debug output
        std::cout << boost::format("D3D11Hooker::CreateDXGIFactory1_Hook: "
          "Error! %s.") %e.what() << std::endl;
            
        // Failure
        return E_FAIL;
      }
    }
        
    // dxgi.dll!IDXGIFactory::CreateSwapChain hook implementation
    HRESULT WINAPI D3D11Hooker::CreateSwapChain_Hook(
      IDXGIFactory* pThis,
      IUnknown *pDevice,
      DXGI_SWAP_CHAIN_DESC *pDesc,
      IDXGISwapChain **ppSwapChain)
    {
      // Function is not thread-safe
      static boost::mutex MyMutex;
      boost::lock_guard<boost::mutex> MyLock(MyMutex);
        
      try
      {
        // Call trampoline
        typedef HRESULT (WINAPI* tCreateSwapChain)(
          IDXGIFactory* pThis,
          IUnknown *pDevice,
          DXGI_SWAP_CHAIN_DESC *pDesc,
          IDXGISwapChain **ppSwapChain);
        auto pCreateSwapChain = reinterpret_cast<tCreateSwapChain>(
          m_pCreateSwapChainHk->GetTrampoline());
        HRESULT Result = pCreateSwapChain(pThis, pDevice, pDesc, ppSwapChain);
        
        // Debug output
        std::wcout << boost::wformat(L"D3D11Hooker::CreateSwapChain_Hook: "
          L"pDevice = %p, pDesc = %p, ppSwapChain = %p. Return = %u.") 
          %pDevice %pDesc %ppSwapChain %Result << std::endl;
            
        // Hook if call successful
        if (SUCCEEDED(Result) && !m_pPresentHk)
        {
          // Get pointer to dxgi.dll!IDXGISwapChain::Present
          PVOID* pSwapChainVMT = *reinterpret_cast<PVOID**>(*ppSwapChain);
          PVOID pPresent = pSwapChainVMT[8];
          
          // Debug output
          std::wcout << boost::wformat(L"D3D11Hooker::CreateSwapChain_Hook: "
            L"Hooking dxgi.dll!IDXGISwapChain::Present (%p).") 
            %pCreateSwapChain << std::endl;

          // Hook dxgi.dll!IDXGISwapChain::Present
          Memory::MemoryMgr const MyMemory(GetCurrentProcessId());
          m_pPresentHk.reset(new Hades::Memory::PatchDetour(MyMemory, 
            pPresent, &Present_Hook));
          m_pPresentHk->Apply();
        }
        
        // Return result from trampoline
        return Result;
      }
      catch (std::exception const& e)
      {
        // Debug output
        std::cout << boost::format("D3D11Hooker::CreateSwapChain_Hook: "
          "Error! %s.") %e.what() << std::endl;
            
        // Failure
        return E_FAIL;
      }
    }
    
    // dxgi.dll!IDXGISwapChain::Present hook implementation
    HRESULT WINAPI D3D11Hooker::Present_Hook(
      IDXGISwapChain* pThis,
      UINT SyncInterval,
      UINT Flags)
    {
      // Function is not thread-safe
      static boost::mutex MyMutex;
      boost::lock_guard<boost::mutex> MyLock(MyMutex);
        
      try
      {
        // Get trampoline
        typedef HRESULT (WINAPI* tPresent)(
        IDXGISwapChain* pThis,
          UINT SyncInterval,
          UINT Flags);
        auto pPresent = reinterpret_cast<tPresent>(m_pPresentHk->
          GetTrampoline());
        
        // Notify kernel of OnFrame event
        if (m_pRenderer)
        {
          m_pKernel->OnFrame(*m_pRenderer);
        }
        else
        {
          std::wcout << "D3D11Hooker::Present_Hook: Warning! Attempt to "
            "notify Kernel of OnFrame event without a valid renderer." 
            << std::endl;
        }
        
        // Call trampoline
        HRESULT Result = pPresent(pThis, SyncInterval, Flags);
        
        // Return result from trampoline
        return Result;
      }
      catch (std::exception const& e)
      {
        // Debug output
        std::cout << boost::format("D3D11Hooker::Present_Hook: "
          "Error! %s.") %e.what() << std::endl;
            
        // Failure
        return E_FAIL;
      }
    }
  }
}

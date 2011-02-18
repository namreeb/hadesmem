/*
This file is part of HadesMem.
Copyright (C) 2010 Joshua Boyce (aka RaptorFactor, Cypherjb, Cypher, Chazwazza).
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

// Hades
#include "Device.hpp"
#include "Hooker.hpp"
#include "Factory.hpp"
#include "SwapChain.hpp"
#include "DeviceContext.hpp"
#include "HadesCommon/Logger.hpp"

namespace Hades
{
  namespace D3D11
  {
    // Todo: Ensure hook removal on module unload
    
    Kernel::Kernel* D3D11Hooker::m_pKernel = nullptr;
    
    std::shared_ptr<Hades::Memory::PatchDetour> D3D11Hooker::m_pD3D11CreateDeviceAndSwapChainHk;
      
    std::shared_ptr<Hades::Memory::PatchDetour> D3D11Hooker::m_pCreateDXGIFactoryHk;
      
    std::shared_ptr<Hades::Memory::PatchDetour> D3D11Hooker::m_pCreateDXGIFactory1Hk;
      
    void D3D11Hooker::Initialize(Kernel::Kernel& MyKernel)
    {
      m_pKernel = &MyKernel;
    }
     
    void D3D11Hooker::Hook()
    {
      HMODULE D3D11Mod = LoadLibrary(L"d3d11.dll");
      
      FARPROC pD3D11CreateDeviceAndSwapChain = GetProcAddress(D3D11Mod, 
        "D3D11CreateDeviceAndSwapChain");
      if (!pD3D11CreateDeviceAndSwapChain)
      {
        HADES_LOG_THREAD_SAFE(std::wcout << L"D3D11Hooker::Hook: Could not "
          L"find D3D11!D3D11CreateDeviceAndSwapChain." << std::endl);
      }
      else
      {
        HADES_LOG_THREAD_SAFE(std::wcout << L"D3D11Hooker::Hook: Hooking "
          L"D3D11!D3D11CreateDeviceAndSwapChain." << std::endl);
        Hades::Memory::MemoryMgr MyMemory(GetCurrentProcessId());
        m_pD3D11CreateDeviceAndSwapChainHk.reset(new Hades::Memory::PatchDetour(
          MyMemory, pD3D11CreateDeviceAndSwapChain, 
          &D3D11CreateDeviceAndSwapChain_Hook));
        m_pD3D11CreateDeviceAndSwapChainHk->Apply();
      }
      
      HMODULE DXGIMod = LoadLibrary(L"dxgi.dll");
      
      FARPROC pCreateDXGIFactory = GetProcAddress(DXGIMod, 
        "CreateDXGIFactory");
      if (!pCreateDXGIFactory)
      {
        HADES_LOG_THREAD_SAFE(std::wcout << L"D3D11Hooker::Hook: Could not "
          L"find DXGI!CreateDXGIFactory." << std::endl);
      }
      else
      {
        HADES_LOG_THREAD_SAFE(std::wcout << L"D3D11Hooker::Hook: Hooking DXGI!"
          L"CreateDXGIFactory." << std::endl);
        Hades::Memory::MemoryMgr MyMemory(GetCurrentProcessId());
        m_pCreateDXGIFactoryHk.reset(new Hades::Memory::PatchDetour(MyMemory, 
          pCreateDXGIFactory, &CreateDXGIFactory_Hook));
        m_pCreateDXGIFactoryHk->Apply();
      }
      
      FARPROC pCreateDXGIFactory1 = GetProcAddress(DXGIMod, 
        "CreateDXGIFactory1");
      if (!pCreateDXGIFactory1)
      {
        HADES_LOG_THREAD_SAFE(std::wcout << L"D3D11Hooker::Hook: Could not "
          L"find DXGI!CreateDXGIFactory1." << std::endl);
      }
      else
      {
        HADES_LOG_THREAD_SAFE(std::wcout << L"D3D11Hooker::Hook: Hooking DXGI!"
          L"CreateDXGIFactory1." << std::endl);
        Hades::Memory::MemoryMgr MyMemory(GetCurrentProcessId());
        m_pCreateDXGIFactory1Hk.reset(new Hades::Memory::PatchDetour(MyMemory, 
          pCreateDXGIFactory1, &CreateDXGIFactory1_Hook));
        m_pCreateDXGIFactory1Hk->Apply();
      }
    }
    
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
    }
    
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
      HADES_LOG_THREAD_SAFE(std::wcout << 
        L"D3D11Hooker::D3D11CreateDeviceAndSwapChain_Hook: Called." 
        << std::endl);
        
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
      auto const pD3D11CreateDeviceAndSwapChain = 
        reinterpret_cast<tD3D11CreateDeviceAndSwapChain>(
        m_pD3D11CreateDeviceAndSwapChainHk->GetTrampoline());
      HRESULT Ret = pD3D11CreateDeviceAndSwapChain(pAdapter, DriverType, 
        Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, 
        pSwapChainDesc, ppSwapChain, ppDevice, pFeatureLevel, 
        ppImmediateContext);
        
      if (SUCCEEDED(Ret))
      {
        HADES_LOG_THREAD_SAFE(std::wcout << 
          L"D3D11Hooker::D3D11CreateDeviceAndSwapChain_Hook: "
          L"Call successful." << std::endl);
        
        auto pDevice = ppDevice ? *ppDevice : nullptr;
        auto pImmediateContext = ppImmediateContext ? *ppImmediateContext : 
          nullptr;
          
        if (ppDevice && ppImmediateContext)
        {
          HADES_LOG_THREAD_SAFE(std::wcout << 
            L"D3D11Hooker::D3D11CreateDeviceAndSwapChain_Hook: "
            L"Hooking ID3D11Device." << std::endl);
          *ppDevice = new ID3D11DeviceHook(*m_pKernel, 
            pDevice, pImmediateContext);
          
          HADES_LOG_THREAD_SAFE(std::wcout << 
            L"D3D11Hooker::D3D11CreateDeviceAndSwapChain_Hook: "
            L"Hooking ID3D11DeviceContext." << std::endl);
          *ppImmediateContext = new ID3D11DeviceContextHook(
            pImmediateContext);
        }
      }
      else
      {
        HADES_LOG_THREAD_SAFE(std::wcout << 
          L"D3D11Hooker::D3D11CreateDevice_Hook: Call failed." << std::endl);
      }
      
      return Ret;
    }
    
    HRESULT WINAPI D3D11Hooker::CreateDXGIFactory_Hook(
      REFIID riid,
      void **ppFactory)
    {
      HADES_LOG_THREAD_SAFE(std::wcout << 
        L"D3D11Hooker::CreateDXGIFactory_Hook: Called." 
        << std::endl);
        
      typedef HRESULT (WINAPI* tCreateDXGIFactory)(
        REFIID riid,
        void **ppFactory);
      auto pCreateDXGIFactory = reinterpret_cast<tCreateDXGIFactory>(
        m_pCreateDXGIFactoryHk->GetTrampoline());
      HRESULT Ret = pCreateDXGIFactory(riid, ppFactory);
      if (SUCCEEDED(Ret))
      {
        HADES_LOG_THREAD_SAFE(std::wcout << 
          L"D3D11Hooker::CreateDXGIFactory_Hook: "
          L"Call successful." << std::endl);
            
        HADES_LOG_THREAD_SAFE(std::wcout << 
          L"D3D11Hooker::CreateDXGIFactory_Hook: "
          L"Hooking IDXGIFactory." << std::endl);
            
        *ppFactory = new IDXGIFactoryHook(reinterpret_cast<IDXGIFactory*>(
          *ppFactory));
      }
      else
      {
        HADES_LOG_THREAD_SAFE(std::wcout << 
          L"D3D11Hooker::CreateDXGIFactory_Hook: "
          L"Call failed." << std::endl);
      }
      
      return Ret;
    }
    
    HRESULT WINAPI D3D11Hooker::CreateDXGIFactory1_Hook(
      REFIID riid,
      void **ppFactory)
    {
      HADES_LOG_THREAD_SAFE(std::wcout << 
        L"D3D11Hooker::CreateDXGIFactory1_Hook: Called." 
        << std::endl);
        
      typedef HRESULT (WINAPI* tCreateDXGIFactory1)(
        REFIID riid,
        void **ppFactory);
      auto pCreateDXGIFactory1 = reinterpret_cast<tCreateDXGIFactory1>(
        m_pCreateDXGIFactory1Hk->GetTrampoline());
      HRESULT Ret = pCreateDXGIFactory1(riid, ppFactory);
      if (SUCCEEDED(Ret))
      {
        HADES_LOG_THREAD_SAFE(std::wcout << 
          L"D3D11Hooker::CreateDXGIFactory1_Hook: "
          L"Call successful." << std::endl);
            
        HADES_LOG_THREAD_SAFE(std::wcout << 
          L"D3D11Hooker::CreateDXGIFactory1_Hook: "
          L"Hooking IDXGIFactory1." << std::endl);
            
        *ppFactory = new IDXGIFactory1Hook(reinterpret_cast<IDXGIFactory1*>(
          *ppFactory));
      }
      else
      {
        HADES_LOG_THREAD_SAFE(std::wcout << 
          L"D3D11Hooker::CreateDXGIFactory1_Hook: "
          L"Call failed." << std::endl);
      }
      
      return Ret;
    }
  }
}

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
#include "Exports.hpp"
#include "SwapChain.hpp"
#include "DeviceContext.hpp"
#include "HadesCommon/Logger.hpp"

namespace Hades
{
  namespace D3D11
  {
    std::shared_ptr<Hades::Memory::PatchDetour> D3D11CreateDeviceAndSwapChainHk;
      
    std::shared_ptr<Hades::Memory::PatchDetour> CreateDXGIFactoryHk;
      
    class EnsureRemoveHook
    {
    public:
      explicit EnsureRemoveHook(std::shared_ptr<Hades::Memory::PatchDetour> 
        Hook) 
        : m_Hook(Hook)
      { }
      
      ~EnsureRemoveHook()
      {
        if (m_Hook)
        {
          m_Hook->Remove();
        }
      }
    
    private:
      std::shared_ptr<Hades::Memory::PatchDetour> m_Hook;
    };
    
    EnsureRemoveHook D3D11CreateDeviceAndSwapChainHk_Cleanup(
      D3D11CreateDeviceAndSwapChainHk);
    EnsureRemoveHook CreateDXGIFactoryHk_Cleanup(CreateDXGIFactoryHk);
      
    HRESULT WINAPI D3D11CreateDeviceAndSwapChain_Hook(
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
      ID3D11DeviceContext **ppImmediateContext
    );
    
    HRESULT WINAPI CreateDXGIFactory_Hook(
      REFIID riid,
      void **ppFactory
    );
    
    void HookD3D11()
    {
      HMODULE D3D11Mod = LoadLibrary(L"d3d11.dll");
      
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
        ID3D11DeviceContext **ppImmediateContext
      );
      
      auto pD3D11CreateDeviceAndSwapChain = 
        reinterpret_cast<tD3D11CreateDeviceAndSwapChain>(GetProcAddress(
        D3D11Mod, "D3D11CreateDeviceAndSwapChain"));
      if (!pD3D11CreateDeviceAndSwapChain)
      {
        HADES_LOG_THREAD_SAFE(std::wcout << L"HookD3D11: Could not find "
          L"D3D11!D3D11CreateDeviceAndSwapChain." << std::endl);
      }
      else
      {
        HADES_LOG_THREAD_SAFE(std::wcout << L"HookD3D11: Hooking D3D11!"
          L"D3D11CreateDeviceAndSwapChain." << std::endl);
        Hades::Memory::MemoryMgr MyMemory(GetCurrentProcessId());
        D3D11CreateDeviceAndSwapChainHk.reset(new Hades::Memory::PatchDetour(
          MyMemory, pD3D11CreateDeviceAndSwapChain, 
          &D3D11CreateDeviceAndSwapChain_Hook));
        D3D11CreateDeviceAndSwapChainHk->Apply();
      }
      
      HMODULE DXGIMod = LoadLibrary(L"dxgi.dll");
      
      typedef HRESULT (WINAPI* tCreateDXGIFactory)(
        REFIID riid,
        void **ppFactory
      );
      
      auto pCreateDXGIFactory = reinterpret_cast<tCreateDXGIFactory>(
        GetProcAddress(DXGIMod, "CreateDXGIFactory"));
      if (!pCreateDXGIFactory)
      {
        HADES_LOG_THREAD_SAFE(std::wcout << L"HookD3D11: Could not find DXGI!"
          L"CreateDXGIFactory." << std::endl);
      }
      else
      {
        HADES_LOG_THREAD_SAFE(std::wcout << L"HookD3D11: Hooking DXGI!"
          L"CreateDXGIFactory." << std::endl);
        Hades::Memory::MemoryMgr MyMemory(GetCurrentProcessId());
        CreateDXGIFactoryHk.reset(new Hades::Memory::PatchDetour(MyMemory, 
          pCreateDXGIFactory, &CreateDXGIFactory_Hook));
        CreateDXGIFactoryHk->Apply();
      }
    }
    
    HRESULT WINAPI D3D11CreateDeviceAndSwapChain_Hook(
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
      ID3D11DeviceContext **ppImmediateContext
    )
    {
      HADES_LOG_THREAD_SAFE(std::wcout << 
        L"D3D11CreateDeviceAndSwapChain_Hook: Test." << std::endl);
        
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
        ID3D11DeviceContext **ppImmediateContext
      );
      auto pD3D11CreateDeviceAndSwapChain = 
        reinterpret_cast<tD3D11CreateDeviceAndSwapChain>(
        D3D11CreateDeviceAndSwapChainHk->GetTrampoline());
      HRESULT Ret = pD3D11CreateDeviceAndSwapChain(pAdapter, DriverType, 
        Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, 
        pSwapChainDesc, ppSwapChain, ppDevice, pFeatureLevel, 
        ppImmediateContext);
        
      if (SUCCEEDED(Ret))
      {
        HADES_LOG_THREAD_SAFE(std::wcout << 
          L"D3D11CreateDeviceAndSwapChain_Hook: "
          L"Call successful." << std::endl);
            
        if (ppSwapChain)
        {
          HADES_LOG_THREAD_SAFE(std::wcout << 
            L"D3D11CreateDeviceAndSwapChain_Hook: "
            L"Hooking IDXGISwapChain." << std::endl);
          *ppSwapChain = new IDXGISwapChainHook(
            ppDevice ? *ppDevice : nullptr, 
            *ppSwapChain, pSwapChainDesc);
        }
        if (ppDevice)
        {
          HADES_LOG_THREAD_SAFE(std::wcout << 
            L"D3D11CreateDeviceAndSwapChain_Hook: "
            L"Hooking ID3D11Device." << std::endl);
          *ppDevice = new ID3D11DeviceHook(*ppDevice);
        }
        if (ppImmediateContext)
        {
          HADES_LOG_THREAD_SAFE(std::wcout << 
            L"D3D11CreateDeviceAndSwapChain_Hook: "
            L"Hooking ID3D11DeviceContext." << std::endl);
          *ppImmediateContext = new ID3D11DeviceContextHook(
            *ppImmediateContext);
        }
      }
      else
      {
        HADES_LOG_THREAD_SAFE(std::wcout << L"D3D11CreateDevice_Hook: "
          L"Call failed." << std::endl);
      }
      
      return Ret;
    }
    
    HRESULT WINAPI CreateDXGIFactory_Hook(
      REFIID riid,
      void **ppFactory
    )
    {
      HADES_LOG_THREAD_SAFE(std::wcout << L"CreateDXGIFactory_Hook: Test." << 
        std::endl);
        
      typedef HRESULT (WINAPI* tCreateDXGIFactory)(
        REFIID riid,
        void **ppFactory
      );
      auto pCreateDXGIFactory = reinterpret_cast<tCreateDXGIFactory>(
        CreateDXGIFactoryHk->GetTrampoline());
      return pCreateDXGIFactory(riid, ppFactory);
    }
  }
}

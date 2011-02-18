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

#pragma once

// C++ Standard Library
#include <memory>

// Windows API
#include <Windows.h>

// DirectX
#define D3D11_IGNORE_SDK_LAYERS
#include <d3d11.h>

// Hades
#include "HadesMemory/Memory.hpp"
#include "HadesKernel/Kernel.hpp"

namespace Hades
{
  namespace D3D11
  {
    class D3D11Hooker
    {
    public:
      class Error : public virtual HadesError 
      { };
      
      static void Initialize(Kernel::Kernel& MyKernel);

      static void Hook();
      
      static void Unhook();
      
    private:
      static HRESULT WINAPI D3D11CreateDeviceAndSwapChain_Hook(
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
              
      static HRESULT WINAPI CreateDXGIFactory_Hook(
        REFIID riid,
        void **ppFactory);
      
      static HRESULT WINAPI CreateDXGIFactory1_Hook(
        REFIID riid,
        void **ppFactory);
      
      static Kernel::Kernel* m_pKernel;
        
      static std::shared_ptr<Hades::Memory::PatchDetour> m_pD3D11CreateDeviceAndSwapChainHk;
        
      static std::shared_ptr<Hades::Memory::PatchDetour> m_pCreateDXGIFactoryHk;
        
      static std::shared_ptr<Hades::Memory::PatchDetour> m_pCreateDXGIFactory1Hk;
    };
  }
}

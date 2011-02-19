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
    // D3D11 hooker
    class D3D11Hooker
    {
    public:
      // Error type
      class Error : public virtual HadesError 
      { };
      
      // Initialize D3D11 hooker
      static void Initialize(Kernel::Kernel& MyKernel);

      // Hook D3D11
      static void Hook();
      
      // Unhook D3D11
      static void Unhook();
      
    private:
      // dxgi.dll!CreateDXGIFactory hook implementation
      static HRESULT WINAPI CreateDXGIFactory_Hook(
        REFIID riid,
        void **ppFactory);
      
      // dxgi.dll!CreateDXGIFactory1 hook implementation
      static HRESULT WINAPI CreateDXGIFactory1_Hook(
        REFIID riid,
        void **ppFactory);
        
      // dxgi.dll!IDXGIFactory::CreateSwapChain hook implementation
      static HRESULT WINAPI CreateSwapChain_Hook(
        IUnknown *pDevice,
        DXGI_SWAP_CHAIN_DESC *pDesc,
        IDXGISwapChain **ppSwapChain);
      
      // dxgi.dll!IDXGISwapChain::Present hook implementation
      static HRESULT WINAPI Present_Hook(
        UINT SyncInterval,
        UINT Flags);

      // Kernel instance
      static Kernel::Kernel* m_pKernel;
      
      // dxgi.dll!CreateDXGIFactory hook
      static std::shared_ptr<Hades::Memory::PatchDetour> m_pCreateDXGIFactoryHk;
        
      // dxgi.dll!CreateDXGIFactory1 hook
      static std::shared_ptr<Hades::Memory::PatchDetour> m_pCreateDXGIFactory1Hk;
        
      // dxgi.dll!IDXGIFactory::CreateSwapChain hook
      static std::shared_ptr<Hades::Memory::PatchDetour> m_pCreateSwapChainHk;
        
      // dxgi.dll!IDXGISwapChain::Present hook
      static std::shared_ptr<Hades::Memory::PatchDetour> m_pPresentHk;
    };
  }
}

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
#include <d3d9.h>
#include <d3dx9.h>

// Hades
#include "Renderer.hpp"
#include "HadesMemory/Memory.hpp"
#include "HadesKernel/Kernel.hpp"

namespace Hades
{
  namespace D3D9
  {
    // D3D9 hooker
    class D3D9Hooker
    {
    public:
      // Error type
      class Error : public virtual HadesError 
      { };
      
      // Initialize D3D9 hooker
      static void Initialize(Kernel::Kernel& MyKernel);

      // Hook D3D9
      static void Hook();
      
      // Unhook D3D9
      static void Unhook();
      
    private:
      // d3d9.dll!Direct3DCreate9 hook implementation
      static IDirect3D9* WINAPI Direct3DCreate9_Hook(UINT SDKVersion);
      
      // d3d9.dll!IDirect3D9::CreateDevice hook implementation
      static HRESULT WINAPI CreateDevice_Hook(
        IDirect3D9* pThis, 
        UINT Adapter, 
        D3DDEVTYPE DeviceType, 
        HWND hFocusWindow, 
        DWORD BehaviorFlags, 
        D3DPRESENT_PARAMETERS* pPresentationParameters, 
        IDirect3DDevice9** ppReturnedDeviceInterface);

      // d3d9.dll!IDrect3DDevice9::EndScene hook implementation
      static HRESULT WINAPI EndScene_Hook(
        IDirect3DDevice9* pThis);

      // d3d9.dll!IDrect3DDevice9::Reset hook implementation
      static HRESULT WINAPI Reset_Hook(
        IDirect3DDevice9* pThis, 
        D3DPRESENT_PARAMETERS* pPresentationParameters);

      // Kernel instance
      static Kernel::Kernel* m_pKernel;
        
      // Device instance
      static IDirect3DDevice9* m_pDevice;
      
      // Renderer instance
      static std::shared_ptr<GUI::D3D9Renderer> m_pRenderer;
      
      // d3d9.dll!Direct3DCreate9 hook
      static std::shared_ptr<Hades::Memory::PatchDetour> m_pDirect3DCreate9Hk;
      
      // d3d9.dll!IDirect3D9::CreateDevice hook  
      static std::shared_ptr<Hades::Memory::PatchDetour> m_pCreateDeviceHk;
        
      // d3d9.dll!IDrect3DDevice9::EndScene hook
      static std::shared_ptr<Hades::Memory::PatchDetour> m_pEndSceneHk;
      // d3d9.dll!IDrect3DDevice9::Reset hook
      static std::shared_ptr<Hades::Memory::PatchDetour> m_pResetHk;
    };
  }
}

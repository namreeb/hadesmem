/*
This file is part of HadesMem.
Copyright © 2010 RaptorFactor (aka Cypherjb, Cypher, Chazwazza). 
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

// D3D9
#include <d3d9.h>

// Hades
#include "HadesMemory/Memory.hpp"

namespace Hades
{
  namespace Kernel
  {
    class D3D9Hook
    {
    public:
      // D3D9Hook exception type
      class Error : public virtual HadesError 
      { };

      // Start D3D9 engine
      static void Startup();
      
      // Stop D3D9 engine
      static void Shutdown();
    
    private:
      // IDrect3DDevice9::EndScene hook implementation
      static HRESULT WINAPI EndScene_Hook(IDirect3DDevice9* pThis);

      // IDrect3DDevice9::Reset hook implementation
      static HRESULT WINAPI Reset_Hook(IDirect3DDevice9* pThis, 
        D3DPRESENT_PARAMETERS* pPresentationParameters);

      // Initialize resources such as textures (managed and unmanaged), vertex 
      // buffers, and other D3D resources
      static void Initialize();

      // Release all unmanaged resources
      static void PreReset();

      // Re-initialize all unmanaged resources
      static void PostReset();

      // Do rendering
      static void Render();

      // Current device
      static IDirect3DDevice9* m_pDevice;

      // State block
      static IDirect3DStateBlock9*	m_pStateBlock;

      // Hooks
      static std::shared_ptr<Memory::PatchDetour> m_pResetHk;
      static std::shared_ptr<Memory::PatchDetour> m_pEndSceneHk;
    };
  }
}

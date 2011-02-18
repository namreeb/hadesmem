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

// Windows API
#include <Windows.h>

// DirectX
#include <d3d9.h>
#include <d3dx9.h>

// Hades
#include "HadesMemory/Memory.hpp"
#include "HadesKernel/Kernel.hpp"

namespace Hades
{
  namespace D3D9
  {
    class D3D9Hooker
    {
    public:
      class Error : public virtual HadesError 
      { };
      
      static void Initialize(Kernel::Kernel& MyKernel);

      static void Hook();
      
      static void Unhook();
      
    private:
      static IDirect3D9* WINAPI Direct3DCreate9_Hook(UINT SDKVersion);
      
      static Kernel::Kernel* m_pKernel;
        
      static std::shared_ptr<Hades::Memory::PatchDetour> m_pDirect3DCreate9;
    };
  }
}

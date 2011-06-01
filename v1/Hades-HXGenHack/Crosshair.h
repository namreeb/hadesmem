/*
This file is part of HadesMem.
Copyright © 2010 Cypherjb (aka Chazwazza, aka Cypher). 
<http://www.cypherjb.com/> <cypher.jb@gmail.com>

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

// Hades
#include "Hades-D3D9/D3D9Mgr.h"

namespace Hades
{
  namespace HXGenHack
  {
    // Crosshair exception type
    class CrosshairError : public virtual HadesError 
    { };

    // Crosshair managing class.
    class Crosshair
    {
    public:
      // Initialize crosshair
      static void Startup(Kernel::Kernel* pKernel);

      // Enable crosshair
      static void Enable();
      
      // Disable crosshair
      static void Disable();

    private:
      // OnFrame callback (for crosshair drawing)
      static void OnFrame(IDirect3DDevice9* pDevice, 
        D3D9::D3D9HelperPtr pHelper);

      // Whether crosshar is enabled
      static bool m_Enabled;
    };
  }
}

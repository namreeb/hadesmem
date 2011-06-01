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

// Hades
#include "Hades-Kernel/Kernel.h"
#include "Hades-Memory/Patcher.h"

// C++ Standard Library
#include <memory>

namespace Hades
{
  namespace HXGenHack
  {
    // Speeder exception type
    class SpeederError : public virtual HadesError 
    { };

    // Speedhack managing class.
    class Speeder
    {
    public:
      // Initialize speeder
      static void Startup(Kernel::Kernel* pKernel);

      // Get speed multiplier
      static DWORD GetSpeed();

      // Set speed multiplier
      static void SetSpeed(DWORD Multiplier);

    private:
      // QueryPerformanceCounter hook function
      static BOOL WINAPI QueryPerformanceCounter_Hook(
        LARGE_INTEGER* lpPerformanceCount);

      // GetTickCount hook function
      static DWORD WINAPI GetTickCount_Hook();

      // Speed multiplier
      static DWORD m_Multiplier;

      // QueryPerformanceCounter hook
      static std::shared_ptr<Hades::Memory::PatchDetour> 
        m_pQueryPerformanceCounterHk;

      // GetTickCount hook
      static std::shared_ptr<Hades::Memory::PatchDetour> m_pGetTickCountHk;
    };
  }
}

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

// Windows API
#include <Windows.h>

// C++ Standard Library
#include <vector>

// Boost
#pragma warning(push, 1)
#include <boost/noncopyable.hpp>
#pragma warning(pop)

// Hades
#include "Fwd.h"
#include "Error.h"
#include "MemoryMgr.h"

namespace Hades
{
  namespace Memory
  {
    // Patch class.
    // Abstract base class for different patch types.
    class Patch
    {
    public:
      // Patch exception type
      class Error : public virtual HadesMemError
      { };

      // Constructor
      explicit Patch(MemoryMgr const& MyMemory);

      // Destructor
      virtual ~Patch();

      // Apply patch
      virtual void Apply() = 0;
      // Remove patch
      virtual void Remove() = 0;

      // Whether patch is currently applied
      bool IsApplied() const;

    protected:
      // Memory manager instance
      MemoryMgr m_Memory;

      // Whether patch is currently applied
      bool m_Applied;
    };

    // Raw patch (a.k.a. 'byte patch').
    // Used to perform a simple byte patch on a target.
    class PatchRaw : public Patch
    {
    public:
      // Constructor
      PatchRaw(MemoryMgr const& MyMemory, PVOID Target, 
        std::vector<BYTE> const& Data);

      // Apply patch
      virtual void Apply();

      // Remove patch
      virtual void Remove();

    private:
      // Patch target
      PVOID m_Target;

      // New data
      std::vector<BYTE> m_Data;

      // Original data
      std::vector<BYTE> m_Orig;
    };

    // Detour patch (a.k.a. 'hook').
    // Performs an 'inline' or 'jump' hook on the target.
    class PatchDetour : public Patch
    {
    public:
      // Constructor
      PatchDetour(MemoryMgr const& MyMemory, PVOID Target, PVOID Detour);

      // Apply patch
      virtual void Apply();
      
      // Remove patch
      virtual void Remove();

      // Get pointer to trampoline
      PVOID GetTrampoline() const;

    private:
      // Write jump to target at address
      void WriteJump(PVOID Address, PVOID Target);

      // Get size of jump instruction for current platform
      unsigned int GetJumpSize() const;

      // Target address
      PVOID m_Target;
      // Detour address
      PVOID m_Detour;
      // Trampoline address
      PVOID m_Trampoline;
      // Backup code
      std::vector<BYTE> m_Orig;
    };
  }
}

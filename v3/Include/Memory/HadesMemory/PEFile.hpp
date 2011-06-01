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

// Windows
#include <Windows.h>

// Hades
#include "Fwd.hpp"
#include "Error.hpp"
#include "MemoryMgr.hpp"

namespace Hades
{
  namespace Memory
  {
    // PE file format wrapper
    class PeFile
    {
    public:
      // PeFile exception type
      class Error : public virtual HadesMemError 
      { };

      enum FileType
      {
        FileType_Image, 
        FileType_Data
      };

      // Constructor
      PeFile(MemoryMgr const& MyMemory, PVOID Address, 
        FileType Type = FileType_Image);

      // Get memory manager
      MemoryMgr GetMemoryMgr() const;

      // Get base address
      PBYTE GetBase() const;

      // Convert RVA to VA
      PVOID RvaToVa(DWORD Rva) const;

      // Get file type
      FileType GetType() const;

    protected:
      // Memory instance
      MemoryMgr m_Memory;

      // Base address
      PBYTE m_pBase;

      // File type
      FileType m_Type;
    };
  }
}

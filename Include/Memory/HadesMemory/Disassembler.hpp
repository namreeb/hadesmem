/*
This file is part of HadesMem.
Copyright (C) 2011 Joshua Boyce (a.k.a. RaptorFactor).
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

// Hades
#include "HadesCommon/Config.hpp"

// C++ Standard Library
#include <string>
#include <vector>

// Windows API
#include <Windows.h>

// BeaEngine
#ifdef HADES_MSVC
#pragma warning(push, 1)
#endif
#include "BeaEngine/BeaEngine.h"
#ifdef HADES_MSVC
#pragma warning(pop)
#endif

// Hades
#include "HadesMemory/Fwd.hpp"
#include "HadesMemory/Error.hpp"
#include "HadesMemory/MemoryMgr.hpp"

namespace Hades
{
  namespace Memory
  {
    // Disassembler data
    struct DisasmData
    {
      DISASM Disasm;
      int Len;
      std::vector<BYTE> Raw;
    };

    // Disassembler managing class
    class Disassembler
    {
    public:
      // Disassembler exception type
      class Error : public virtual HadesMemError 
      { };

      // Constructor
      explicit Disassembler(MemoryMgr const& MyMemory);

      // Disassemble target to string
      std::vector<std::wstring> DisassembleToStr(PVOID Address, 
        DWORD_PTR NumInstructions) const;

      // Disassemble target
      std::vector<DisasmData> Disassemble(PVOID Address, 
        DWORD_PTR NumInstructions) const;

    private:
      // MemoryMgr instance
      MemoryMgr m_MemoryMgr;
    };
  }
}

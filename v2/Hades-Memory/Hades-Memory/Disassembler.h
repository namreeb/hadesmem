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
#include <string>
#include <vector>

// Boost
#pragma warning(push, 1)
#pragma warning (disable: ALL_CODE_ANALYSIS_WARNINGS)
#include <boost/noncopyable.hpp>
#pragma warning(pop)

// BeaEngine
#pragma warning(push, 1)
#pragma warning (disable: ALL_CODE_ANALYSIS_WARNINGS)
#include "BeaEngine/BeaEngine.h"
#pragma warning(pop)

// Hades
#include "Fwd.h"
#include "Error.h"
#include "MemoryMgr.h"

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

      // Disassemble target and get results as strings
      std::vector<std::string> DisassembleToStr(PVOID Address, 
        DWORD_PTR NumInstructions) const;

      // Disassemble target and get full disasm data back
      std::vector<DisasmData> Disassemble(PVOID Address, 
        DWORD_PTR NumInstructions) const;

    private:
      // MemoryMgr instance
      MemoryMgr m_MemoryMgr;
    };
  }
}

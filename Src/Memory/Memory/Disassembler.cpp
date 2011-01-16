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

// C++ Standard Library
#include <iterator>
#include <algorithm>

// Boost
#include <boost/lexical_cast.hpp>

// Hades
#include "MemoryMgr.hpp"
#include "Disassembler.hpp"
#include "HadesCommon/I18n.hpp"

namespace Hades
{
  namespace Memory
  {
    // Constructor
    Disassembler::Disassembler(MemoryMgr const& MyMemory) 
      : m_MemoryMgr(MyMemory)
    { }

    // Disassemble target to string
    std::vector<std::wstring> Disassembler::DisassembleToStr(
      PVOID Address, DWORD_PTR NumInstructions) const
    {
      // Disassemble target
      std::vector<DisasmData> MyDisasmData(Disassemble(Address, 
        NumInstructions));

      // Container to hold disassembled code as a string
      std::vector<std::wstring> Results;
      std::transform(MyDisasmData.cbegin(), MyDisasmData.cend(), 
        std::back_inserter(Results), 
        [] (DisasmData const& MyDisasm) 
      {
        return boost::lexical_cast<std::wstring>(static_cast<std::string>(
          MyDisasm.Disasm.CompleteInstr));
      });

      // Return disassembled data
      return Results;
    }

    // Disassemble target
    std::vector<DisasmData> Disassembler::Disassemble(PVOID Address, 
      DWORD_PTR NumInstructions) const
    {
      // Read data into buffer
      int MaxInstructionSize = 15;
      std::vector<BYTE> Buffer(m_MemoryMgr.Read<std::vector<BYTE>>(Address, 
        NumInstructions * MaxInstructionSize));

      // Set up disasm structure for BeaEngine
      DISASM MyDisasm = { 0 };
      MyDisasm.EIP = reinterpret_cast<long long>(&Buffer[0]);
      MyDisasm.VirtualAddr = reinterpret_cast<long long>(Address);
#if defined(_M_AMD64) 
      MyDisasm.Archi = 64;
#elif defined(_M_IX86) 
      MyDisasm.Archi = 32;
#else 
#error "[HadesMem] Unsupported architecture."
#endif

      // Container to hold disassembled code as a string
      std::vector<DisasmData> Results;

      // DisassembleToStr instructions
      for (DWORD_PTR i = 0; i < NumInstructions; ++i)
      {
        // DisassembleToStr current instruction
        int const Len = Disasm(&MyDisasm);
        // Ensure disassembly succeeded
        if (Len == UNKNOWN_OPCODE)
        {
          break;
        }

        // Current disasm data
        DisasmData CurData;
        CurData.Disasm = MyDisasm;
        CurData.Len = Len;
        CurData.Raw = m_MemoryMgr.Read<std::vector<BYTE>>(
          reinterpret_cast<PVOID>(MyDisasm.VirtualAddr), Len);

        // Add current instruction to list
        Results.push_back(CurData);

        // Advance to next instruction
        MyDisasm.EIP += Len;
        MyDisasm.VirtualAddr += Len;
      }

      // Return disassembled data
      return Results;
    }
  }
}

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

// Windows API
#include <Windows.h>

// C++ Standard Library
#include <vector>

// Boost
#include <boost/format.hpp>

// HadesMem
#include "Memory.h"
#include "Disassembler.h"

namespace Hades
{
  namespace Memory
  {
    // Patch class.
    // Abstract base class for different patch types.
    class Patch : private boost::noncopyable
    {
    public:
      // Constructor
      inline Patch(MemoryMgr const& MyMemory);
      // Destructor
      inline virtual ~Patch();

      // Apply patch
      inline virtual void Apply() = 0;
      // Remove patch
      inline virtual void Remove() = 0;

      // Whether patch is currently applied
      inline bool IsApplied() const;

    protected:
      // Memory manager instance
      MemoryMgr const& m_Memory;
      // Whether patch is currently applied
      bool m_Applied;
    };

    // Constructor
    Patch::Patch(MemoryMgr const& MyMemory) 
      : m_Memory(MyMemory), 
      m_Applied(false)
    { }

    // Destructor
    Patch::~Patch()
    { }

    // Whether patch is currently applied
    bool Patch::IsApplied() const
    {
      return m_Applied;
    }

    // Raw patch (a.k.a. 'byte patch').
    // Used to perform a simple byte patch on a target.
    class PatchRaw : public Patch
    {
    public:
      // Constructor
      inline PatchRaw(MemoryMgr const& MyMemory, PVOID Target, 
        std::vector<BYTE> Data);

      // Apply patch
      inline virtual void Apply();
      // Remove patch
      inline virtual void Remove();

    private:
      // Patch target
      PVOID m_Target;
      // New data
      std::vector<BYTE> const m_Data;
      // Original data
      std::vector<BYTE> m_Orig;
    };

    // Constructor
    PatchRaw::PatchRaw(MemoryMgr const& MyMemory, PVOID Target, 
      std::vector<BYTE> Data) 
      : Patch(MyMemory), 
      m_Target(Target), 
      m_Data(Data), 
      m_Orig()
    { }

    // Apply patch
    void PatchRaw::Apply()
    {
      // If the patch has already been applied there's nothing left to do
      if (m_Applied)
      {
        return;
      }

      // Backup original data
      m_Orig = m_Memory.Read<std::vector<BYTE>>(m_Target, m_Data.size());
      // Write target data
      m_Memory.Write(m_Target, m_Data);

      // Flush cache
      m_Memory.FlushCache(m_Target, m_Data.size());

      // Patch is applied
      m_Applied = true;
    }

    // Remove patch
    void PatchRaw::Remove()
    {
      // If the patch is not currently applied there's nothing left to do
      if (!m_Applied)
      {
        return;
      }

      // Restore original data
      m_Memory.Write(m_Target, m_Orig);

      // Flush cache
      m_Memory.FlushCache(m_Target, m_Orig.size());

      // Patch is removed
      m_Applied = false;
    }

    // Detour patch (a.k.a. 'hook').
    // Performs an 'inline' or 'jump' hook on the target.
    class PatchDetour : public Patch
    {
    public:
      // Constructor
      inline PatchDetour(MemoryMgr const& MyMemory, PVOID Target, 
        PVOID Detour);

      // Apply patch
      inline virtual void Apply();
      // Remove patch
      inline virtual void Remove();

      // Get pointer to trampoline
      inline PVOID GetTrampoline() const;

    private:
      // Write jump to target at address
      inline void WriteJump(PVOID Address, PVOID Target);

      // Get size of jump instruction for current platform
      inline unsigned int GetJumpSize() const;

      // Target address
      PVOID m_Target;
      // Detour address
      PVOID m_Detour;
      // Trampoline address
      PVOID m_Trampoline;
      // Backup code
      std::vector<BYTE> m_Orig;
    };

    // Constructor
    PatchDetour::PatchDetour(MemoryMgr const& MyMemory, PVOID Target, 
      PVOID Detour) 
      : Patch(MyMemory), 
      m_Target(Target), 
      m_Detour(Detour), 
      m_Trampoline(nullptr)
    { }

    // Apply patch
    void PatchDetour::Apply()
    {
      // If the patch has already been applied there's nothing left to do
      if (m_Applied)
      {
        return;
      }

      // Calculate size of trampoline buffer to generate (for worst case 
      // scenario)
      ULONG const TrampSize = GetJumpSize() * 3;

      // Allocate trampoline buffer
      m_Trampoline = m_Memory.Alloc(TrampSize);
      PBYTE TrampCur = static_cast<PBYTE>(m_Trampoline);

      // Disassemble target (for worst case scenario)
      Disassembler MyDisasm(m_Memory);
      auto MyDisasmData(MyDisasm.Disassemble(m_Target, TrampSize));

      // Parse disassembly
      unsigned int InstrSize = 0;
      for (auto i = MyDisasmData.begin(); i != MyDisasmData.end(); ++i)
      {
        // Break once we've disassembled enough data
        if (InstrSize > GetJumpSize())
        {
          break;
        }

        // Get current instruction data
        DisasmData const& CurDisasmData = *i;
        DISASM const& CurDisasm = CurDisasmData.Disasm;
        int CurLen = CurDisasmData.Len;
        std::vector<BYTE> const& CurRaw = CurDisasmData.Raw;

        // Detect and resolve jumps
        if ((CurDisasm.Instruction.BranchType == JmpType) && 
          (CurDisasm.Instruction.AddrValue != 0)) 
        {
          WriteJump(TrampCur, reinterpret_cast<PVOID>(CurDisasm.
            Instruction.AddrValue));
          TrampCur += GetJumpSize();
        }
        // Handle 'generic' instructions
        else
        {
          m_Memory.Write(TrampCur, CurRaw);
          TrampCur += CurLen;
        }

        // Add to total instruction size
        InstrSize += CurLen;
      }

      // Write jump back to target
      WriteJump(TrampCur, static_cast<PBYTE>(m_Target) + InstrSize);
      TrampCur += GetJumpSize();

      // Flush instruction cache
      m_Memory.FlushCache(m_Trampoline, InstrSize + GetJumpSize());

      // Backup original code
      m_Orig = m_Memory.Read<std::vector<BYTE>>(m_Target, GetJumpSize());

      // Write jump to detour
      WriteJump(m_Target, m_Detour);

      // Flush instruction cache
      m_Memory.FlushCache(m_Target, m_Orig.size());

      // Patch is applied
      m_Applied = true;
    }

    // Remove patch
    void PatchDetour::Remove()
    {
      // If patch hasn't been applied there's nothing left to do
      if (!m_Applied)
      {
        return;
      }

      // Remove detour
      m_Memory.Write(m_Target, m_Orig);

      // Free trampoline
      m_Memory.Free(m_Trampoline);

      // Patch has been removed
      m_Applied = false;
    }

    // Get pointer to trampoline
    PVOID PatchDetour::GetTrampoline() const
    {
      return m_Trampoline;
    }

    // Write jump to target at address
    void PatchDetour::WriteJump(PVOID Address, PVOID Target)
    {
      // Create buffer to hold jump instruction
      std::vector<BYTE> JumpBuf(GetJumpSize());
      // Get pointer to buffer
      PBYTE pJumpBuf = &JumpBuf[0];
      // Write code to buffer ('JMP DWORD PTR [PTR]')
#if defined(_M_AMD64) 
      *pJumpBuf++ = 0xFF;
      *pJumpBuf++ = 0x25;
      *reinterpret_cast<PDWORD>(pJumpBuf) = 0;
      pJumpBuf += sizeof(DWORD);
      *reinterpret_cast<PDWORD_PTR>(pJumpBuf) = reinterpret_cast<DWORD_PTR>(
        Target);
      // Write code to buffer ('JMP TARGET' - REL)
#elif defined(_M_IX86) 
      *pJumpBuf++ = 0xE9;
      *reinterpret_cast<PDWORD_PTR>(pJumpBuf) = static_cast<DWORD_PTR>(
        static_cast<PBYTE>(Target) - static_cast<PBYTE>(Address) - 5);
#else 
#error "Unsupported architecture."
#endif
      // Write code to address
      m_Memory.Write(Address, JumpBuf);
    }

    // Get size of jump instruction for current platform
    unsigned int PatchDetour::GetJumpSize() const
    {
#if defined(_M_AMD64) 
      unsigned int JumpSize = 14;
#elif defined(_M_IX86) 
      unsigned int JumpSize = 5;
#else 
#error "Unsupported architecture."
#endif

      return JumpSize;
    }
  }
}

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

// Hades
#include "Patcher.hpp"
#include "MemoryMgr.hpp"
#include "Disassembler.hpp"

namespace Hades
{
  namespace Memory
  {
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

    // Constructor
    PatchRaw::PatchRaw(MemoryMgr const& MyMemory, PVOID Target, 
      std::vector<BYTE> const& Data) 
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

    // Constructor
    PatchDetour::PatchDetour(MemoryMgr const& MyMemory, PVOID Target, 
      PVOID Detour) 
      : Patch(MyMemory), 
      m_Target(Target), 
      m_Detour(Detour), 
      m_Trampoline()
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
      // Fixme: Ensure trampoline is within range for a RIP-relative 
      // JMP under x64.
      m_Trampoline.reset(new AllocAndFree(m_Memory, TrampSize));
      PBYTE TrampCur = static_cast<PBYTE>(m_Trampoline->GetAddress());

      // Disassemble target (for worst case scenario)
      Disassembler MyDisasm(m_Memory);
      std::vector<DisasmData> MyDisasmData(MyDisasm.Disassemble(m_Target, 
        TrampSize));

      // Parse disassembly
      unsigned int InstrSize = 0;
      for (auto i = MyDisasmData.cbegin(); i != MyDisasmData.cend(); ++i)
      {
        // Break once we've disassembled enough data
        if (InstrSize >= GetJumpSize())
        {
          break;
        }

        // Get current instruction data
        DisasmData const& CurDisasmData = *i;
        DISASM const& CurDisasm = CurDisasmData.Disasm;
        int CurLen = CurDisasmData.Len;
        std::vector<BYTE> const& CurRaw = CurDisasmData.Raw;

        // Detect and resolve jumps
        // Fixme: Resolve other relative instructions.
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
      m_Memory.FlushCache(m_Trampoline->GetAddress(), 
        InstrSize + GetJumpSize());

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
      m_Trampoline.reset();

      // Patch has been removed
      m_Applied = false;
    }

    // Get pointer to trampoline
    PVOID PatchDetour::GetTrampoline() const
    {
      return m_Trampoline->GetAddress();
    }

    // Write jump to target at address
    void PatchDetour::WriteJump(PVOID Address, PVOID Target)
    {
      // Create buffer to hold jump instruction
      std::vector<BYTE> JumpBuf(GetJumpSize());
      // Get pointer to buffer
      PBYTE pJumpBuf = JumpBuf.data();
      // Write code to buffer ('JMP QWORD NEAR [RIP+0]')
      // Fixme: If an attempt to hook an already hooked function is made 
      // the disassembled code will be 'garbage' due to code and data being 
      // mixed. Rewrite this using a safer redirection method.
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
#error "[HadesMem] Unsupported architecture."
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
#error "[HadesMem] Unsupported architecture."
#endif

      return JumpSize;
    }
  }
}

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
#include <HadesMemory/Patcher.hpp>
#include <HadesCommon/Config.hpp>
#include <HadesMemory/MemoryMgr.hpp>

// AsmJit
#ifdef HADES_MSVC
#pragma warning(push, 1)
#endif
#ifdef HADES_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-pedantic"
#pragma GCC diagnostic ignored "-Wattributes"
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#include <AsmJit/AsmJit.h>
#ifdef HADES_MSVC
#pragma warning(pop)
#endif
#ifdef HADES_GCC
#pragma GCC diagnostic pop
#endif

// BeaEngine
#ifdef HADES_MSVC
#pragma warning(push, 1)
#endif
#include <BeaEngine/BeaEngine.h>
#ifdef HADES_MSVC
#pragma warning(pop)
#endif

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
      m_Trampoline.reset(new AllocAndFree(m_Memory, TrampSize));
      PBYTE TrampCur = static_cast<PBYTE>(m_Trampoline->GetBase());
      
      // Get instructions at target
      std::vector<BYTE> Buffer(m_Memory.Read<std::vector<BYTE>>(m_Target, 
        TrampSize));

      // Set up disasm structure for BeaEngine
      DISASM MyDisasm;
      ZeroMemory(&MyDisasm, sizeof(MyDisasm));
      MyDisasm.EIP = reinterpret_cast<long long>(Buffer.data());
      MyDisasm.VirtualAddr = reinterpret_cast<long long>(m_Target);
#if defined(_M_AMD64) 
      MyDisasm.Archi = 64;
#elif defined(_M_IX86) 
      MyDisasm.Archi = 32;
#else 
#error "[HadesMem] Unsupported architecture."
#endif

      // Disassemble instructions until we have enough data to generate the 
      // trampoline
      unsigned int InstrSize = 0;
      while (InstrSize < GetJumpSize())
      {
        // Disassemble target
        int const Len = Disasm(&MyDisasm);
        // Ensure disassembly succeeded
        if (Len == UNKNOWN_OPCODE)
        {
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("PatchDetour::Apply") << 
            ErrorString("Disassembly failed."));
        }

        // Detect and resolve jumps
        if ((MyDisasm.Instruction.BranchType == JmpType) && 
          (MyDisasm.Instruction.AddrValue != 0)) 
        {
          WriteJump(TrampCur, reinterpret_cast<PVOID>(MyDisasm.
            Instruction.AddrValue));
          TrampCur += GetJumpSize();
        }
        // Handle 'generic' instructions
        else
        {
          auto CurRaw = m_Memory.Read<std::vector<BYTE>>(
            reinterpret_cast<PVOID>(MyDisasm.VirtualAddr), Len);
          m_Memory.Write(TrampCur, CurRaw);
          TrampCur += Len;
        }

        // Advance to next instruction
        MyDisasm.EIP += Len;
        MyDisasm.VirtualAddr += Len;

        // Add to total instruction size
        InstrSize += Len;
      }

      // Write jump back to target
      WriteJump(TrampCur, static_cast<PBYTE>(m_Target) + InstrSize);
      TrampCur += GetJumpSize();

      // Flush instruction cache
      m_Memory.FlushCache(m_Trampoline->GetBase(), InstrSize + GetJumpSize());

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
      return m_Trampoline->GetBase();
    }

    // Write jump to target at address
    void PatchDetour::WriteJump(PVOID Address, PVOID Target)
    {
      // Create Assembler.
      AsmJit::Assembler MyJitFunc;

      // Write code to buffer
      // PUSH <Low Absolute>
      // MOV [RSP+4], <High Absolute>
      // RET
#if defined(_M_AMD64) 
      MyJitFunc.push(static_cast<DWORD>(reinterpret_cast<DWORD_PTR>(Target)));
      MyJitFunc.mov(AsmJit::dword_ptr(AsmJit::rsp, 4), static_cast<DWORD>(((
        reinterpret_cast<DWORD_PTR>(Target) >> 32) & 0xFFFFFFFF)));
      MyJitFunc.ret();
      // Write code to buffer
      // JMP <Target, Relative>
#elif defined(_M_IX86) 
      MyJitFunc.jmp(Target);
#else 
#error "[HadesMem] Unsupported architecture."
#endif

      // Get stub size
      DWORD_PTR const StubSize = MyJitFunc.getCodeSize();
      
      // Ensure stub size is as expected
      if (StubSize != GetJumpSize())
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("PatchDetour::WriteJump") << 
          ErrorString("Unexpected stub size."));
      }

      // Create buffer to hold jump instruction
      std::vector<BYTE> JumpBuf(StubSize);
        
      // Generate code
      MyJitFunc.relocCode(JumpBuf.data(), reinterpret_cast<DWORD_PTR>(
        Address));

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

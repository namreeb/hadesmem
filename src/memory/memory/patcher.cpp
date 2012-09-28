// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/patcher.hpp"

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <asmjit/asmjit.h>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <beaengine/BeaEngine.h>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/alloc.hpp"
#include "hadesmem/error.hpp"
#include "hadesmem/read.hpp"
#include "hadesmem/write.hpp"
#include "hadesmem/process.hpp"

// TODO: Replace FlushInstructionCache calls with a wrapper.
// TODO: Class function hooking (ecx preservation). (xchg ecx, [esp]; push ecx)
// TODO: Fastcall function hooking (ecx and edx preservation).
// TODO: VEH hooking (INT3, DR, invalid instruction, etc).
// TODO: Improved relative instruction rebuilding (incl conditionals). x64 has 
// far more relative instructions than x86.
// TODO: VMT hooking.
// TODO: IAT/EAT hooking.
// TODO: Hotpatching support for Windows API code (and other code compatible 
// with hotpatching). Allows thread-safe hooking.
// TODO: Explicitly support hook chains (and test).
// TODO: Use relative jumps where possible (detect delta at runtime).
// TODO: Detect cases where hooking may overflow past the end of a function, 
// and fail (provide policy or flag to allow overriding this behaviour). 
// Examples may be instructions such as INT3, RET, JMP, etc.

namespace hadesmem
{

Patch::Patch(Process const* process) 
  : process_(process), 
  applied_(false)
{ }

Patch::Patch(Patch&& other)
  : process_(other.process_), 
  applied_(other.applied_)
{
  other.process_ = nullptr;
  other.applied_ = false;
}

Patch& Patch::operator=(Patch&& other)
{
  process_ = other.process_;
  other.process_ = nullptr;
  applied_ = other.applied_;
  other.applied_ = false;
  return *this;
}

Patch::~Patch()
{ }

bool Patch::IsApplied() const
{
  return applied_;
}

PatchRaw::PatchRaw(Process const* process, PVOID target, 
  std::vector<BYTE> const& data) 
  : Patch(process), 
  target_(target), 
  data_(data), 
  orig_()
{ }

PatchRaw::PatchRaw(PatchRaw&& other)
  : Patch(std::forward<Patch>(other)), 
  target_(other.target_), 
  data_(std::move(other.data_)), 
  orig_(std::move(other.orig_))
{
  other.target_ = nullptr;
}

PatchRaw& PatchRaw::operator=(PatchRaw&& other)
{
  Patch::operator=(std::forward<Patch>(other));
  target_ = other.target_;
  other.target_ = nullptr;
  data_ = std::move(other.data_);
  orig_ = std::move(other.orig_);
  return *this;
}

PatchRaw::~PatchRaw()
{ }

void PatchRaw::Apply()
{
  if (applied_)
  {
    return;
  }

  orig_ = ReadVector<BYTE>(*process_, target_, data_.size());

  WriteVector(*process_, target_, data_);

  if (!FlushInstructionCache(process_->GetHandle(), target_, data_.size()))
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("FlushInstructionCache failed.") << 
      ErrorCodeWinLast(last_error));
  }

  applied_ = true;
}

void PatchRaw::Remove()
{
  if (!applied_)
  {
    return;
  }

  WriteVector(*process_, target_, orig_);

  if (!FlushInstructionCache(process_->GetHandle(), target_, orig_.size()))
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("FlushInstructionCache failed.") << 
      ErrorCodeWinLast(last_error));
  }

  applied_ = false;
}

PatchDetour::PatchDetour(Process const* process, PVOID target, LPCVOID detour) 
  : Patch(process), 
  target_(target), 
  detour_(detour), 
  trampoline_(), 
  orig_()
{ }

PatchDetour::PatchDetour(PatchDetour&& other)
  : Patch(std::forward<Patch>(other)), 
  target_(other.target_), 
  detour_(other.detour_), 
  trampoline_(std::move(other.trampoline_)), 
  orig_(std::move(other.orig_))
{
  other.target_ = nullptr;
  other.detour_ = nullptr;
}

PatchDetour& PatchDetour::operator=(PatchDetour&& other)
{
  Patch::operator=(std::forward<Patch>(other));
  target_ = other.target_;
  other.target_ = nullptr;
  detour_ = other.detour_;
  other.detour_ = nullptr;
  trampoline_ = std::move(other.trampoline_);
  orig_ = std::move(other.orig_);
  return *this;
}

PatchDetour::~PatchDetour()
{ }

void PatchDetour::Apply()
{
  if (applied_)
  {
    return;
  }

  ULONG const trampoline_len = GetJumpSize() * 3;

  trampoline_.reset(new Allocator(process_, trampoline_len));
  PBYTE tramp_cur = static_cast<PBYTE>(trampoline_->GetBase());

  std::vector<BYTE> buffer(ReadVector<BYTE>(*process_, target_, 
    trampoline_len));

  DISASM my_disasm;
  ZeroMemory(&my_disasm, sizeof(my_disasm));
  my_disasm.EIP = reinterpret_cast<long long>(buffer.data());
  my_disasm.VirtualAddr = reinterpret_cast<long long>(target_);
#if defined(_M_AMD64) 
  my_disasm.Archi = 64;
#elif defined(_M_IX86) 
  my_disasm.Archi = 32;
#else 
#error "[HadesMem] Unsupported architecture."
#endif

  unsigned int instr_size = 0;
  do
  {
    int const len = Disasm(&my_disasm);
    if (len == UNKNOWN_OPCODE)
    {
      BOOST_THROW_EXCEPTION(HadesMemError() << 
        ErrorString("Disassembly failed."));
    }

    // TODO: Support more types of relative instructions
    if ((my_disasm.Instruction.BranchType == JmpType) && 
      (my_disasm.Instruction.AddrValue != 0)) 
    {
      WriteJump(tramp_cur, reinterpret_cast<PVOID>(my_disasm.
        Instruction.AddrValue));
      tramp_cur += GetJumpSize();
    }
    else
    {
      auto cur_raw = ReadVector<BYTE>(*process_, 
        reinterpret_cast<PVOID>(my_disasm.VirtualAddr), len);
      WriteVector(*process_, tramp_cur, cur_raw);
      tramp_cur += len;
    }

    my_disasm.EIP += len;
    my_disasm.VirtualAddr += len;

    instr_size += len;
  } while (instr_size < GetJumpSize());

  WriteJump(tramp_cur, static_cast<PBYTE>(target_) + instr_size);
  tramp_cur += GetJumpSize();

  if (!FlushInstructionCache(process_->GetHandle(), trampoline_->GetBase(),
    instr_size + GetJumpSize()))
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("FlushInstructionCache failed.") << 
      ErrorCodeWinLast(last_error));
  }

  orig_ = ReadVector<BYTE>(*process_, target_, GetJumpSize());

  WriteJump(target_, detour_);

  if (!FlushInstructionCache(process_->GetHandle(), target_, orig_.size()))
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("FlushInstructionCache failed.") << 
      ErrorCodeWinLast(last_error));
  }
  
  applied_ = true;
}

void PatchDetour::Remove()
{
  if (!applied_)
  {
    return;
  }

  WriteVector(*process_, target_, orig_);

  trampoline_.reset();

  applied_ = false;
}

LPCVOID PatchDetour::GetTrampoline() const
{
  return trampoline_->GetBase();
}

void PatchDetour::WriteJump(PVOID address, LPCVOID target)
{
  AsmJit::X86Assembler assembler;

#if defined(_M_AMD64) 
  // PUSH <Low Absolute>
  // MOV [RSP+4], <High Absolute>
  // RET
  assembler.push(static_cast<DWORD>(reinterpret_cast<DWORD_PTR>(target)));
  assembler.mov(AsmJit::dword_ptr(AsmJit::rsp, 4), static_cast<DWORD>(((
    reinterpret_cast<DWORD_PTR>(target) >> 32) & 0xFFFFFFFF)));
  assembler.ret();
#elif defined(_M_IX86) 
  // JMP <Target, Relative>
  assembler.jmp(reinterpret_cast<DWORD_PTR>(target));
#else 
#error "[HadesMem] Unsupported architecture."
#endif

  DWORD_PTR const stub_size = assembler.getCodeSize();

  if (stub_size != GetJumpSize())
  {
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Unexpected stub size."));
  }

  std::vector<BYTE> jump_buf(stub_size);

  assembler.relocCode(jump_buf.data(), reinterpret_cast<DWORD_PTR>(
    address));

  WriteVector(*process_, address, jump_buf);
}

unsigned int PatchDetour::GetJumpSize() const
{
#if defined(_M_AMD64) 
  unsigned int jump_size = 14;
#elif defined(_M_IX86) 
  unsigned int jump_size = 5;
#else 
#error "[HadesMem] Unsupported architecture."
#endif

  return jump_size;
}


}

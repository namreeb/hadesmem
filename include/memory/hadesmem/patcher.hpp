// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <locale>
#include <memory>
#include <vector>
#include <climits>
#include <cstdint>
#include <sstream>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <asmjit/asmjit.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <udis86.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <windows.h>

#include <hadesmem/read.hpp>
#include <hadesmem/alloc.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/flush.hpp>
#include <hadesmem/write.hpp>
#include <hadesmem/process.hpp>

// TODO: Fix the code so this hack can be removed.
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wweak-vtables"
#endif

namespace hadesmem
{
  class Patch
  {
  public:
    explicit Patch(Process const& process)
      : process_(&process), 
      applied_(false)
    { }
    
    Patch(Patch&& other)
      : process_(other.process_), 
      applied_(other.applied_)
    {
      other.applied_ = false;
    }
    
    Patch& operator=(Patch&& other)
    {
      process_ = other.process_;
      applied_ = other.applied_;
      other.applied_ = false;
      return *this;
    }

    virtual ~Patch()
    { }

    virtual void Apply() = 0;
    
    virtual void Remove() = 0;

    bool IsApplied() const
    {
      return applied_;
    }

  protected:
    Process const* process_;

    bool applied_;
    
  private:
    Patch(Patch const&);
    Patch& operator=(Patch const&);
  };

  class PatchRaw : public Patch
  {
  public:
    PatchRaw(Process const& process, 
      PVOID target, 
      std::vector<BYTE> const& data)
      : Patch(process), 
      target_(target), 
      data_(data), 
      orig_()
    { }
    
    PatchRaw(PatchRaw&& other)
      : Patch(std::forward<Patch>(other)), 
      target_(other.target_), 
      data_(std::move(other.data_)), 
      orig_(std::move(other.orig_))
    {
      other.target_ = nullptr;
    }
    
    PatchRaw& operator=(PatchRaw&& other)
    {
      Patch::operator=(std::forward<Patch>(other));
      target_ = other.target_;
      other.target_ = nullptr;
      data_ = std::move(other.data_);
      orig_ = std::move(other.orig_);
      return *this;
    }
        
    virtual ~PatchRaw()
    { }

    virtual void Apply()
    {
      if (applied_)
      {
        return;
      }

      orig_ = ReadVector<BYTE>(*process_, target_, data_.size());
      
      WriteVector(*process_, target_, data_);

      FlushInstructionCache(*process_, target_, data_.size());

      applied_ = true;
    }

    virtual void Remove()
    {
      if (!applied_)
      {
        return;
      }

      WriteVector(*process_, target_, orig_);

      FlushInstructionCache(*process_, target_, orig_.size());

      applied_ = false;
    }

  private:
    PVOID target_;
    std::vector<BYTE> data_;
    std::vector<BYTE> orig_;
  };

  class PatchDetour : public Patch
  {
  public:
    PatchDetour(Process const& process, PVOID target, PVOID detour)
      : Patch(process), 
      target_(target), 
      detour_(detour), 
      trampoline_()
    { }
    
    PatchDetour(PatchDetour&& other)
      : Patch(std::forward<Patch>(other)), 
      target_(other.target_), 
      detour_(other.detour_), 
      trampoline_(std::move(other.trampoline_)), 
      orig_(std::move(other.orig_))
    {
      other.target_ = nullptr;
      other.detour_ = nullptr;
    }
    
    PatchDetour& operator=(PatchDetour&& other)
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
        
    virtual ~PatchDetour()
    { }

    virtual void Apply()
    {
      if (applied_)
      {
        return;
      }

      // Calculate size of trampoline buffer to generate (for worst case 
      // scenario)
      // TODO: Fix this to use maximum instruction length instead? (15 on 
      // both x86 and x64)
      ULONG const tramp_size = GetJumpSize() * 3;

      trampoline_.reset(new Allocator(*process_, tramp_size));
      PBYTE tramp_cur = static_cast<PBYTE>(trampoline_->GetBase());
    
      std::vector<BYTE> buffer(ReadVector<BYTE>(*process_, target_, 
        tramp_size));

      ud_t ud_obj;
      ud_init(&ud_obj);
      ud_set_input_buffer(&ud_obj, buffer.data(), buffer.size());
      ud_set_syntax(&ud_obj, UD_SYN_INTEL);
      ud_set_pc(&ud_obj, reinterpret_cast<std::uint64_t>(target_));
#if defined(_M_AMD64) 
      ud_set_mode(&ud_obj, 64);
#elif defined(_M_IX86) 
      ud_set_mode(&ud_obj, 32);
#else 
#error "[HadesMem] Unsupported architecture."
#endif

      unsigned int instr_size = 0;
      do
      {
        unsigned int const len = ud_disassemble(&ud_obj);
        if (len == 0)
        {
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorString("Disassembly failed."));
        }
        
#ifndef NDEBUG
        char const* const asm_str = ud_insn_asm(&ud_obj);
        std::string asm_str_full = "PatchDetour::Apply: ";
        asm_str_full += (asm_str ? asm_str : "Disassembly printing failed.");
        asm_str_full += ' ';
        asm_str_full += '[';
        char const* const asm_bytes_str = ud_insn_hex(&ud_obj);
        asm_str_full += (asm_bytes_str ? asm_bytes_str : 
          "Disassembly bytes printing failed.");
        asm_str_full += ']';
        asm_str_full += '\n';
        OutputDebugStringA(asm_str_full.c_str());
#endif

        // TODO: Support more types of relative instructions
        // TODO: Support other operand sizes.
        ud_operand_t const* op = ud_insn_opr(&ud_obj, 0);
        std::size_t const sdword_size_bits = sizeof(std::int32_t) * CHAR_BIT;
        if (ud_obj.mnemonic == UD_Ijmp && 
          op && 
          op->type == UD_OP_JIMM && 
          op->size == sdword_size_bits)
        {
          std::uint64_t const insn_base = ud_insn_off(&ud_obj);
          std::int32_t const insn_target = op->lval.sdword;
          unsigned int const insn_len = ud_insn_len(&ud_obj);
          PVOID jump_target = reinterpret_cast<PBYTE>(
            static_cast<DWORD_PTR>(insn_base)) + insn_target + insn_len;
#ifndef NDEBUG
          std::stringstream jmp_ss;
          jmp_ss.imbue(std::locale::classic());
          jmp_ss << std::hex << jump_target;
          std::string const jump_str = "Jump target is " + jmp_ss.str() + 
            ".\n";
          OutputDebugStringA(jump_str.c_str());
#endif
          WriteJump(tramp_cur, jump_target);
          tramp_cur += GetJumpSize();
        }
        else
        {
          uint8_t const* raw = ud_insn_ptr(&ud_obj);
          Write(*process_, tramp_cur, raw, raw + len);
          tramp_cur += len;
        }

        instr_size += len;
      } while (instr_size < GetJumpSize());

      WriteJump(tramp_cur, static_cast<PBYTE>(target_) + instr_size);
      tramp_cur += GetJumpSize();

      FlushInstructionCache(*process_, trampoline_->GetBase(), 
        instr_size + GetJumpSize());

      orig_ = ReadVector<BYTE>(*process_, target_, GetJumpSize());

      WriteJump(target_, detour_);

      FlushInstructionCache(*process_, target_, orig_.size());

      applied_ = true;
    }
    
    virtual void Remove()
    {
      if (!applied_)
      {
        return;
      }

      WriteVector(*process_, target_, orig_);

      trampoline_.reset();

      applied_ = false;
    }

    PVOID GetTrampoline() const
    {
      return trampoline_->GetBase();
    }

  private:
    void WriteJump(PVOID address, PVOID target)
    {
      AsmJit::X86Assembler jit;

#if defined(_M_AMD64) 
      // PUSH <Low Absolute>
      // MOV [RSP+4], <High Absolute>
      // RET
      jit.push(static_cast<DWORD>(reinterpret_cast<DWORD_PTR>(target)));
      jit.mov(AsmJit::dword_ptr(AsmJit::rsp, 4), static_cast<DWORD>(((
        reinterpret_cast<DWORD_PTR>(target) >> 32) & 0xFFFFFFFF)));
      jit.ret();
#elif defined(_M_IX86) 
      // JMP <Target, Relative>
      jit.jmp(target);
#else 
#error "[HadesMem] Unsupported architecture."
#endif

      DWORD_PTR const stub_size = jit.getCodeSize();
      if (stub_size != GetJumpSize())
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorString("Unexpected stub size."));
      }

      std::vector<BYTE> jump_buf(stub_size);
      
      jit.relocCode(jump_buf.data(), reinterpret_cast<DWORD_PTR>(address));

      WriteVector(*process_, address, jump_buf);
    }

    unsigned int GetJumpSize() const
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

    PVOID target_;
    PVOID detour_;
    std::unique_ptr<Allocator> trampoline_;
    std::vector<BYTE> orig_;
  };
}

#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic pop
#endif

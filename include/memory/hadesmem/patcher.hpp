// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <climits>
#include <cstdint>
#include <locale>
#include <memory>
#include <sstream>
#include <vector>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <asmjit/asmjit.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <udis86.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <windows.h>

#include <hadesmem/alloc.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/flush.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/thread_helpers.hpp>
#include <hadesmem/write.hpp>

// TODO: Fix exception safety.

// TODO: EAT hooking.

// TODO: IAT hooking.

// TODO: VEH hooking. (INT 3, DR, invalid instr, etc.)

// TODO: VMT hooking.

// TODO: Make hooking a transactional operation.

// TODO: When patching memory, check each thread context to ensure we're not
// overwriting code that's currently being executed in a way that could cause
// a crash. In the case of PatchDetour we should set the thread context to
// redirect it to our trampoline, and in the case of PatchRaw it's probably
// safer just to bail and make the caller try again. Need to document that
// the API may fail spuriously and that the caller should retry.

// TODO: Explicitly support (and test) hook chains.

// TODO: Support 'safe' unloading by incrementing/decrementing a counter for
// each detour so it can be detect when your code is currently executing
// before unloading? What other options are there?

// TODO: Support passing a hook context. (This is needed to support
// multi-module support properly in base hook. i.e. Two concurrent D3D
// instances.) Need to be sure not to dirty registers though.

// TODO: Rewrite to not use AsmJit.

// TODO: Add proper tests for edge cases trying to be handled (thread
// suspension, thread redirection, instruction resolution, no free trampoline
// blocks near a target address, short and far jumps, etc etc.)

// TODO: Use a second trampoline in patcher when jumping to detour to pass a
// hook context (containing original trampoline, original module, etc).

// TODO: Most detours should be declared as extern "C".

// TODO: Improve type safety.

// TODO: Review, refactor, rewrite, etc this entire module. Put TODOs where
// appropriate, remove and add APIs, fix bugs, clean up code, etc. Use new
// language features like noexcept, constexpr, etc. Consider other designs
// entirely.

namespace hadesmem
{

class PatchRaw
{
public:
  explicit PatchRaw(Process const& process,
                    PVOID target,
                    std::vector<BYTE> const& data)
    : process_(&process), applied_(false), target_(target), data_(data), orig_()
  {
  }

  PatchRaw(PatchRaw const& other) = delete;

  PatchRaw& operator=(PatchRaw const& other) = delete;

  PatchRaw(PatchRaw&& other)
    : process_(other.process_),
      applied_(other.applied_),
      target_(other.target_),
      data_(std::move(other.data_)),
      orig_(std::move(other.orig_))
  {
    other.process_ = nullptr;
    other.applied_ = false;
    other.target_ = nullptr;
  }

  PatchRaw& operator=(PatchRaw&& other)
  {
    RemoveUnchecked();

    process_ = other.process_;
    other.process_ = nullptr;

    applied_ = other.applied_;
    other.applied_ = false;

    target_ = other.target_;
    other.target_ = nullptr;

    data_ = std::move(other.data_);

    orig_ = std::move(other.orig_);

    return *this;
  }

  ~PatchRaw()
  {
    RemoveUnchecked();
  }

  bool IsApplied() const HADESMEM_DETAIL_NOEXCEPT
  {
    return applied_;
  }

  void Apply()
  {
    if (applied_)
    {
      return;
    }

    SuspendedProcess const suspended_process(process_->GetId());

    orig_ = ReadVector<BYTE>(*process_, target_, data_.size());

    WriteVector(*process_, target_, data_);

    FlushInstructionCache(*process_, target_, data_.size());

    applied_ = true;
  }

  void Remove()
  {
    if (!applied_)
    {
      return;
    }

    SuspendedProcess const suspended_process(process_->GetId());

    WriteVector(*process_, target_, orig_);

    FlushInstructionCache(*process_, target_, orig_.size());

    applied_ = false;
  }

private:
  // TODO: Code smell... This feels like code duplication.
  void RemoveUnchecked() HADESMEM_DETAIL_NOEXCEPT
  {
    try
    {
      Remove();
    }
    catch (...)
    {
      // WARNING: Patch may not be removed if Remove fails.
      HADESMEM_DETAIL_TRACE_A(
        boost::current_exception_diagnostic_information().c_str());
      HADESMEM_DETAIL_ASSERT(false);

      // TODO: Code smell... Reduce code duplication somehow?
      process_ = nullptr;
      applied_ = false;

      target_ = nullptr;
      data_.clear();
      orig_.clear();
    }
  }

  Process const* process_;
  bool applied_;
  PVOID target_;
  std::vector<BYTE> data_;
  std::vector<BYTE> orig_;
};

// TODO: Consolidate memory allocations where possible. Taking a page for
// every trampoline (including two trampolines per patch on x64 -- fix
// this too) is extremely wasteful. Perhaps allocate a block the size of
// the allocation granularity then use a custom heap?
// TODO: Support calling convention differences for target function and
// detour function (calling convention only though, not args or return
// type). Templates can be used to detect information if appropriate.
class PatchDetour
{
public:
  // TODO: Template for function type(s).
  explicit PatchDetour(Process const& process, PVOID target, PVOID detour)
    : process_(&process),
      applied_(false),
      target_(target),
      detour_(detour),
      trampoline_(),
      orig_(),
      trampolines_()
  {
  }

  PatchDetour(PatchDetour const& other) = delete;

  PatchDetour& operator=(PatchDetour const& other) = delete;

  PatchDetour(PatchDetour&& other)
    : process_(other.process_),
      applied_(other.applied_),
      target_(other.target_),
      detour_(other.detour_),
      trampoline_(std::move(other.trampoline_)),
      orig_(std::move(other.orig_)),
      trampolines_(std::move(other.trampolines_))
  {
    other.process_ = nullptr;
    other.applied_ = false;
    other.target_ = nullptr;
    other.detour_ = nullptr;
  }

  PatchDetour& operator=(PatchDetour&& other)
  {
    RemoveUnchecked();

    process_ = other.process_;
    other.process_ = nullptr;

    applied_ = other.applied_;
    other.applied_ = false;

    target_ = other.target_;
    other.target_ = nullptr;

    detour_ = other.detour_;
    other.detour_ = nullptr;

    trampoline_ = std::move(other.trampoline_);

    orig_ = std::move(other.orig_);

    trampolines_ = std::move(other.trampolines_);

    return *this;
  }

  ~PatchDetour()
  {
    RemoveUnchecked();
  }

  // TODO: Detect when applying or removing patch and code is currently
  // being executed. (Redirect EIP to trampoline.)
  void Apply()
  {
    if (applied_)
    {
      return;
    }

    SuspendedProcess const suspended_process(process_->GetId());

    std::uint32_t const kMaxInstructionLen = 15;
    std::uint32_t const kTrampSize = kMaxInstructionLen * 3;

    trampoline_ = std::make_unique<Allocator>(*process_, kTrampSize);
    PBYTE tramp_cur = static_cast<PBYTE>(trampoline_->GetBase());

    std::vector<BYTE> const buffer(
      ReadVector<BYTE>(*process_, target_, kTrampSize));

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

    bool detour_near = IsNear(target_, detour_);
    HADESMEM_DETAIL_TRACE_A(detour_near ? "Detour near." : "Detour far.");
    DWORD_PTR const jump_size = detour_near ? kJumpSize32 : kJumpSize64;

    // TODO: Detect cases where hooking may overflow past the end of
    // a function, and fail. (Provide policy or flag to allow
    // overriding this behaviour.) Examples may be instructions such
    // as int 3, ret, jmp, etc.
    std::uint32_t instr_size = 0;
    do
    {
      std::uint32_t const len = ud_disassemble(&ud_obj);
      if (len == 0)
      {
        HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                        << ErrorString("Disassembly failed."));
      }

#if !defined(HADESMEM_NO_TRACE)
      char const* const asm_str = ud_insn_asm(&ud_obj);
      char const* const asm_bytes_str = ud_insn_hex(&ud_obj);
      HADESMEM_DETAIL_TRACE_FORMAT_A(
        "%s. [%s].",
        (asm_str ? asm_str : "Invalid."),
        (asm_bytes_str ? asm_bytes_str : "Invalid."));
#endif

      // TODO: Improved relative instruction rebuilding (including
      // conditionals). x64 has far more IP relative instructions
      // than x86.
      // TODO: Support more operand sizes for existing relative
      // instruction support.
      // TODO: Improve instruction rebuilding for cases such as
      // jumps backwards into the detour and fail safely (or
      // whatever is appropriate).
      ud_operand_t const* const op = ud_insn_opr(&ud_obj, 0);
      std::size_t const sdword_size_bits = sizeof(std::int32_t) * CHAR_BIT;
      if ((ud_obj.mnemonic == UD_Ijmp || ud_obj.mnemonic == UD_Icall) && op &&
          op->type == UD_OP_JIMM && op->size == sdword_size_bits)
      {
        std::uint64_t const insn_base = ud_insn_off(&ud_obj);
        std::int32_t const insn_target = op->lval.sdword;
        std::uint32_t const insn_len = ud_insn_len(&ud_obj);
        PVOID const jump_target =
          reinterpret_cast<PBYTE>(static_cast<DWORD_PTR>(insn_base)) +
          insn_target + insn_len;
        HADESMEM_DETAIL_TRACE_FORMAT_A("Jump target is 0x%p.", jump_target);
        if (ud_obj.mnemonic == UD_Ijmp)
        {
          tramp_cur += WriteJump(tramp_cur, jump_target);
        }
        else
        {
          HADESMEM_DETAIL_ASSERT(ud_obj.mnemonic == UD_Icall);
          tramp_cur += WriteCall(tramp_cur, jump_target);
        }
      }
      else
      {
        uint8_t const* const raw = ud_insn_ptr(&ud_obj);
        Write(*process_, tramp_cur, raw, raw + len);
        tramp_cur += len;
      }

      instr_size += len;
    } while (instr_size < jump_size);

    tramp_cur += WriteJump(tramp_cur, static_cast<PBYTE>(target_) + instr_size);

    FlushInstructionCache(
      *process_, trampoline_->GetBase(), trampoline_->GetSize());

    orig_ = ReadVector<BYTE>(*process_, target_, jump_size);

    WriteJump(target_, detour_);

    FlushInstructionCache(*process_, target_, instr_size);

    applied_ = true;
  }

  void Remove()
  {
    if (!applied_)
    {
      return;
    }

    SuspendedProcess const suspended_process(process_->GetId());

    WriteVector(*process_, target_, orig_);

    trampoline_.reset();

    trampolines_.clear();

    applied_ = false;
  }

  PVOID GetTrampoline() const HADESMEM_DETAIL_NOEXCEPT
  {
    return trampoline_->GetBase();
  }

  template <typename FuncT> FuncT GetTrampoline() const HADESMEM_DETAIL_NOEXCEPT
  {
    return reinterpret_cast<FuncT>(
      reinterpret_cast<DWORD_PTR>(trampoline_->GetBase()));
  }

private:
  // TODO: Code smell... This feels like code duplication.
  void RemoveUnchecked() HADESMEM_DETAIL_NOEXCEPT
  {
    try
    {
      Remove();
    }
    catch (...)
    {
      // WARNING: Patch may not be removed if Remove fails.
      HADESMEM_DETAIL_TRACE_A(
        boost::current_exception_diagnostic_information().c_str());
      HADESMEM_DETAIL_ASSERT(false);

      // TODO: Code smell... Reduce code duplication somehow?
      process_ = nullptr;
      applied_ = false;

      target_ = nullptr;
      detour_ = nullptr;
      trampoline_.reset();
      orig_.clear();
      trampolines_.clear();
    }
  }

  // Inspired by EasyHook.
  // TODO: Allow specifying a size rather than only supporting a single page.
  std::unique_ptr<Allocator> AllocatePageNear(PVOID address)
  {
    SYSTEM_INFO sys_info;
    ZeroMemory(&sys_info, sizeof(sys_info));
    GetSystemInfo(&sys_info);
    DWORD const page_size = sys_info.dwPageSize;

#if defined(_M_AMD64)
    LONG_PTR const search_beg = (std::max)(
      reinterpret_cast<LONG_PTR>(address) - 0x7FFFFF00LL,
      reinterpret_cast<LONG_PTR>(sys_info.lpMinimumApplicationAddress));
    LONG_PTR const search_end = (std::min)(
      reinterpret_cast<LONG_PTR>(address) + 0x7FFFFF00LL,
      reinterpret_cast<LONG_PTR>(sys_info.lpMaximumApplicationAddress));

    std::unique_ptr<Allocator> trampoline;

    auto const allocate_tramp = [](Process const & process,
                                   PVOID addr,
                                   SIZE_T size)->std::unique_ptr<Allocator>
    {
      try
      {
        return std::make_unique<Allocator>(process, size, addr);
      }
      catch (std::exception const& /*e*/)
      {
        return std::unique_ptr<Allocator>();
      }
    };

    for (LONG_PTR base = reinterpret_cast<LONG_PTR>(address), index = 0;
         base + index < search_end || base - index > search_beg;
         index += page_size)
    {
      LONG_PTR const higher = base + index;
      if (higher < search_end)
      {
        if (trampoline = allocate_tramp(
              *process_, reinterpret_cast<PVOID>(higher), page_size))
        {
          break;
        }
      }

      LONG_PTR const lower = base - index;
      if (lower > search_beg)
      {
        if (trampoline = allocate_tramp(
              *process_, reinterpret_cast<PVOID>(lower), page_size))
        {
          break;
        }
      }
    }

    if (!trampoline)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error() << ErrorString("Failed to find trampoline memory block."));
    }

    return trampoline;
#elif defined(_M_IX86)
    (void)address;
    return std::make_unique<Allocator>(*process_, page_size);
#else
#error "[HadesMem] Unsupported architecture."
#endif
  }

  bool IsNear(PVOID address, PVOID target)
  {
#if defined(_M_AMD64)
    std::ptrdiff_t const offset =
      static_cast<PBYTE>(address) - static_cast<PBYTE>(target);
    bool const is_near = (offset > -(static_cast<LONG_PTR>(1) << 31)) &&
                         (offset < (static_cast<LONG_PTR>(1) << 31));
    return is_near;
#elif defined(_M_IX86)
    (void)address;
    (void)target;
    return true;
#else
#error "[HadesMem] Unsupported architecture."
#endif
  }

  // TODO: Remove code duplication from WriteCall.
  DWORD_PTR WriteJump(PVOID address, PVOID target)
  {
    AsmJit::X86Assembler jit;
    DWORD_PTR expected_stub_size = 0;
    std::vector<BYTE> jump_buf;
    bool asmjit_bug = false;

#if defined(_M_AMD64)
    if (IsNear(address, target))
    {
      // JMP <Target, Relative>
      jit.jmp(target);

      // AsmJit generates the correct JMP instruction, but then
      // adds a bunch of zero padding for no particular reason...
      // Investigate this.
      // TODO: Fix this.
      expected_stub_size = 0x13;
      asmjit_bug = true;
    }
    else
    {
      // TODO: Fall back to PUSH/RET trick (14 bytes) if finding a
      // trampoline fails.
      // TODO: Try and 'share' trampoline pages where possible.
      // It's a waste of VA space to use an entire page per
      // trampoline when all we store is a single pointer.
      std::unique_ptr<Allocator> trampoline = AllocatePageNear(address);
      PVOID tramp_addr = trampoline->GetBase();

      Write(*process_, tramp_addr, target);

      trampolines_.emplace_back(std::move(trampoline));

      // JMP QWORD PTR <Trampoline, Relative>
      AsmJit::Label label(jit.newLabel());
      jit.bind(label);
      LONG_PTR const disp = static_cast<PBYTE>(tramp_addr) -
                            static_cast<PBYTE>(address) - sizeof(LONG);
      jit.jmp(AsmJit::qword_ptr(label, static_cast<sysint_t>(disp)));

      expected_stub_size = kJumpSize64;
    }

#elif defined(_M_IX86)
    // JMP <Target, Relative>
    jit.jmp(target);

    expected_stub_size = kJumpSize32;
#else
#error "[HadesMem] Unsupported architecture."
#endif

    DWORD_PTR const stub_size = jit.getCodeSize();
    HADESMEM_DETAIL_TRACE_FORMAT_A(
      "Stub size = 0n%Iu, Expected stub size = 0n%Iu.",
      stub_size,
      expected_stub_size);
    // TODO: Should this be an assert?
    if (stub_size != expected_stub_size)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                      << ErrorString("Unexpected stub size."));
    }

    jump_buf.resize(stub_size);

    jit.relocCode(jump_buf.data(), reinterpret_cast<DWORD_PTR>(address));

    // Remove the padding added due to AsmJit bug.
    // TODO: Fix this.
    if (asmjit_bug == true)
    {
      jump_buf.erase(std::begin(jump_buf) + kJumpSize32, std::end(jump_buf));
      HADESMEM_DETAIL_ASSERT(jump_buf.size() == kJumpSize32);
    }

    WriteVector(*process_, address, jump_buf);

    return jump_buf.size();
  }

  DWORD_PTR WriteCall(PVOID address, PVOID target)
  {
    AsmJit::X86Assembler jit;
    DWORD_PTR expected_stub_size = 0;

#if defined(_M_AMD64)
    // TODO: Optimize this to avoid a trampoline where possible.
    // Similar to WriteJump. Try to consolidate the funcs.
    std::unique_ptr<Allocator> trampoline = AllocatePageNear(address);

    PVOID tramp_addr = trampoline->GetBase();

    Write(*process_, tramp_addr, reinterpret_cast<DWORD_PTR>(target));

    trampolines_.emplace_back(std::move(trampoline));

    // CALL QWORD PTR <Trampoline, Relative>
    AsmJit::Label label(jit.newLabel());
    jit.bind(label);
    LONG_PTR const disp = reinterpret_cast<LONG_PTR>(tramp_addr) -
                          reinterpret_cast<LONG_PTR>(address) - sizeof(LONG);
    jit.call(AsmJit::qword_ptr(label, static_cast<sysint_t>(disp)));

    expected_stub_size = kCallSize64;
#elif defined(_M_IX86)
    // CALL <Target, Relative>
    jit.call(target);

    expected_stub_size = kCallSize32;
#else
#error "[HadesMem] Unsupported architecture."
#endif

    DWORD_PTR const stub_size = jit.getCodeSize();
    HADESMEM_DETAIL_TRACE_FORMAT_A(
      "Stub size = 0n%Iu, Expected stub size = 0n%Iu.",
      stub_size,
      expected_stub_size);
    // TODO: Should this be an assert?
    if (stub_size != expected_stub_size)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                      << ErrorString("Unexpected stub size."));
    }

    std::vector<BYTE> jump_buf(stub_size);

    jit.relocCode(jump_buf.data(), reinterpret_cast<DWORD_PTR>(address));

    WriteVector(*process_, address, jump_buf);

    return stub_size;
  }

  static DWORD_PTR const kJumpSize32 = 5;
  static DWORD_PTR const kCallSize32 = 5;
#if defined(_M_AMD64)
  static DWORD_PTR const kJumpSize64 = 6;
  static DWORD_PTR const kCallSize64 = 6;
#elif defined(_M_IX86)
  static DWORD_PTR const kJumpSize64 = kJumpSize32;
  static DWORD_PTR const kCallSize64 = kCallSize32;
#else
#error "[HadesMem] Unsupported architecture."
#endif

  Process const* process_;
  bool applied_;
  PVOID target_;
  PVOID detour_;
  std::unique_ptr<Allocator> trampoline_;
  std::vector<BYTE> orig_;
  std::vector<std::unique_ptr<Allocator>> trampolines_;
};
}

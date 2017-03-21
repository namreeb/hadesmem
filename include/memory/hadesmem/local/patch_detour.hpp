// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <atomic>
#include <climits>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <type_traits>
#include <utility>
#include <vector>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <udis86.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <windows.h>

#include <hadesmem/alloc.hpp>
#include <hadesmem/detail/alias_cast.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/patch_code_gen.hpp>
#include <hadesmem/detail/patch_detour_stub.hpp>
#include <hadesmem/detail/patcher_aux.hpp>
#include <hadesmem/detail/scope_warden.hpp>
#include <hadesmem/detail/srw_lock.hpp>
#include <hadesmem/detail/thread_aux.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/detail/type_traits.hpp>
#include <hadesmem/detail/winternl.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/flush.hpp>
#include <hadesmem/local/patch_detour_base.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/thread.hpp>
#include <hadesmem/thread_list.hpp>
#include <hadesmem/thread_helpers.hpp>
#include <hadesmem/write.hpp>

// TODO: Make hooking a transactional operation (i.e. fix exception guarantees
// etc.).

// TODO: Consolidate all memory allocations where possible. Using a minimum of
// one page (and potentially multiple pages) for each detour is extremely
// wasteful and could be causing memory pressure in some applications. Perhaps
// some sort of remote heap that we can share across all detour types.

// TODO: Add new Symbol class (or something along those lines) for looking up
// function addresses in system DLLs that we want to hook.

// TODO: Avoid some allocations when generating a trampoline by scanning for
// code caves which we can use to store the trampoline.

// TODO: Add proper tests for this (and other patchers). Especially important
// for edge cases trying to be handled (thread suspension, thread redirection,
// instruction resolution, no free trampoline blocks near a target address,
// short and far jumps, etc.).

// TODO: Redesign and rewrite the Patcher collection of APIs.

// TODO: Detect cases where hooking may overflow past the end of a function, and
// fail. (Provide policy or flag to allow overriding this behaviour.) Examples
// may be instructions such as int 3, ret, jmp, etc.

// TODO: Test references, pointers, const, volatile, perfect forwarding, etc.

// TODO: Fix code duplication throughout patcher codebase.

// TODO: Support unwinding through hooks (needs RtlAddFunctionTable for x64?)?

namespace hadesmem
{
template <typename TargetFuncT, typename ContextT = void*>
class PatchDetour : public PatchDetourBase
{
public:
  using TargetFuncRawT =
    std::conditional_t<std::is_member_function_pointer<TargetFuncT>::value,
                       TargetFuncT,
                       std::add_pointer_t<std::remove_pointer_t<TargetFuncT>>>;
  using StubT = detail::PatchDetourStub<TargetFuncT>;
  using DetourFuncRawT = typename StubT::DetourFuncRawT;
  using DetourFuncT = typename StubT::DetourFuncT;

  HADESMEM_DETAIL_STATIC_ASSERT(detail::IsFunction<TargetFuncT>::value);
  HADESMEM_DETAIL_STATIC_ASSERT(detail::IsFunction<TargetFuncRawT>::value);
  HADESMEM_DETAIL_STATIC_ASSERT(detail::IsFunction<DetourFuncRawT>::value);

  explicit PatchDetour(Process const& process,
                       TargetFuncRawT target,
                       DetourFuncT const& detour,
                       ContextT context = ContextT())
    : process_{process},
      target_{detail::AliasCastUnchecked<void*>(target)},
      detour_{detour},
      context_(std::move(context)),
      stub_{std::make_unique<StubT>(this)}
  {
    HADESMEM_DETAIL_ASSERT(target_ != nullptr);
    HADESMEM_DETAIL_ASSERT(detour_ != nullptr);

    if (process.GetId() != ::GetCurrentProcessId())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error{} << ErrorString{"PatchDetour only supported on local process."});
    }
  }

  explicit PatchDetour(Process const&& process,
                       TargetFuncRawT target,
                       DetourFuncT const& detour,
                       ContextT context = ContextT()) = delete;

  PatchDetour(PatchDetour const& other) = delete;

  PatchDetour& operator=(PatchDetour const& other) = delete;

  PatchDetour(PatchDetour&& other)
    : process_{std::move(other.process_)},
      applied_{other.applied_},
      target_{other.target_},
      detour_{std::move(other.detour_)},
      trampoline_{std::move(other.trampoline_)},
      stub_gate_{std::move(other.stub_gate_)},
      orig_(std::move(other.orig_)),
      trampolines_(std::move(other.trampolines_)),
      ref_count_{other.ref_count_.load()},
      stub_{other.stub_},
      context_(std::move(other.context_))
  {
    other.process_ = nullptr;
    other.applied_ = false;
    other.target_ = nullptr;
    other.stub_ = nullptr;
  }

  PatchDetour& operator=(PatchDetour&& other)
  {
    RemoveUnchecked();

    process_ = std::move(other.process_);

    applied_ = other.applied_;
    other.applied_ = false;

    target_ = other.target_;
    other.target_ = nullptr;

    detour_ = std::move(other.detour_);

    trampoline_ = std::move(other.trampoline_);

    stub_gate_ = std::move(other.stub_gate_);

    orig_ = std::move(other.orig_);

    trampolines_ = std::move(other.trampolines_);

    ref_count_ = other.ref_count_.load();

    stub_ = other.stub_;
    other.stub_ = nullptr;

    context_ = std::move(other.context_);

    return *this;
  }

  virtual ~PatchDetour()
  {
    RemoveUnchecked();
  }

  virtual void Apply() override
  {
    if (applied_)
    {
      return;
    }

    if (detached_)
    {
      HADESMEM_DETAIL_ASSERT(false);
      return;
    }

    // Reset the trampolines here because we don't do it in remove, otherwise
    // there's a potential race condition where we want to unhook and unload
    // safely, so we unhook the function, then try waiting on our ref count to
    // become zero, but we haven't actually called the trampoline yet, so we end
    // up jumping to the memory we just free'd!
    trampoline_ = nullptr;
    trampolines_.clear();
    stub_gate_ = nullptr;

    // TODO: Narrow the scope of the suspension to only the point where we
    // actually do the hook, because a thread might hold a lock we need (e.g.
    // heap lock).
    // TODO: Make suspension optional, as in some cases we know that all the
    // threads are suspended already (e.g. creation-time injection).
    // Need to fix the potential deadlock problem, as well as the perf
    // problem, before re-enabling this.
    // SuspendedProcess const suspended_process{process_.GetId()};

    std::uint32_t const kMaxInstructionLen = 15;
    std::uint32_t const kTrampSize = kMaxInstructionLen * 3;

    trampoline_ = std::make_unique<Allocator>(process_, kTrampSize);
    auto tramp_cur = static_cast<std::uint8_t*>(trampoline_->GetBase());

    auto const detour_raw = detour_.target<DetourFuncRawT>();
    (void)detour_raw;
    HADESMEM_DETAIL_TRACE_FORMAT_A(
      "This = %p, Target = %p, Detour = %p, Trampoline = %p.",
      this,
      target_,
      detour_raw,
      trampoline_->GetBase());

    auto const buffer = ReadVector<std::uint8_t>(process_, target_, kTrampSize);

    // TODO: Port to use Capstone instead.
    // TODO: Add a disassembler module to abstract away the actual disassembly
    // library being used (so later it can be replaced with a custom one, or
    // support multiple backends, etc). Also useful when writing other tools
    // like a standalone debugger/disassembler, memory editor, etc.
    ud_t ud_obj;
    ud_init(&ud_obj);
    ud_set_input_buffer(&ud_obj, buffer.data(), buffer.size());
    ud_set_syntax(&ud_obj, UD_SYN_INTEL);
    ud_set_pc(&ud_obj, reinterpret_cast<std::uint64_t>(target_));
#if defined(HADESMEM_DETAIL_ARCH_X64)
    ud_set_mode(&ud_obj, 64);
#elif defined(HADESMEM_DETAIL_ARCH_X86)
    ud_set_mode(&ud_obj, 32);
#else
#error "[HadesMem] Unsupported architecture."
#endif

    stub_gate_ = detail::AllocatePageNear(process_, target_);

    std::size_t const patch_size = GetPatchSize();

    // TODO: Detect backward jumps into the detour and fail/notify/etc.
    std::uint32_t instr_size = 0;
    do
    {
      std::uint32_t const len = ud_disassemble(&ud_obj);
      if (len == 0)
      {
        HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                        << ErrorString{"Disassembly failed."});
      }

#if !defined(HADESMEM_NO_TRACE)
      char const* const asm_str = ud_insn_asm(&ud_obj);
      char const* const asm_bytes_str = ud_insn_hex(&ud_obj);
      HADESMEM_DETAIL_TRACE_FORMAT_A(
        "%s. [%s].",
        (asm_str ? asm_str : "Invalid."),
        (asm_bytes_str ? asm_bytes_str : "Invalid."));
#endif

      // TODO: Improve support for rebuilding relative instructions, especially
      // on x64.
      ud_operand_t const* const op = ud_insn_opr(&ud_obj, 0);
      bool is_jimm = op && op->type == UD_OP_JIMM;
      // Handle JMP QWORD PTR [RIP+Rel32]. Necessary for hook chain support.
      bool is_jmem = op && op->type == UD_OP_MEM && op->base == UD_R_RIP &&
                     op->index == UD_NONE && op->scale == 0 && op->size == 0x40;
      if ((ud_obj.mnemonic == UD_Ijmp || ud_obj.mnemonic == UD_Icall) && op &&
          (is_jimm || is_jmem))
      {
        std::uint16_t const size = is_jimm ? op->size : op->offset;
        HADESMEM_DETAIL_TRACE_FORMAT_A("Operand/offset size is %hu.", size);
        std::int64_t const insn_target = [&]() -> std::int64_t {
          switch (size)
          {
          case sizeof(std::int8_t) * CHAR_BIT:
            return op->lval.sbyte;
          case sizeof(std::int16_t) * CHAR_BIT:
            return op->lval.sword;
          case sizeof(std::int32_t) * CHAR_BIT:
            return op->lval.sdword;
          case sizeof(std::int64_t) * CHAR_BIT:
            return op->lval.sqword;
          default:
            HADESMEM_DETAIL_ASSERT(false);
            HADESMEM_DETAIL_THROW_EXCEPTION(
              Error{} << ErrorString{"Unknown instruction size."});
          }
        }();

        auto const resolve_rel =
          [](std::uint64_t base, std::int64_t target, std::uint32_t insn_len) {
            return reinterpret_cast<std::uint8_t*>(
                     static_cast<std::uintptr_t>(base)) +
                   target + insn_len;
          };

        std::uint64_t const insn_base = ud_insn_off(&ud_obj);
        std::uint32_t const insn_len = ud_insn_len(&ud_obj);

        auto const resolved_target =
          resolve_rel(insn_base, insn_target, insn_len);
        void* const jump_target =
          is_jimm ? resolved_target : Read<void*>(process_, resolved_target);
        HADESMEM_DETAIL_TRACE_FORMAT_A("Jump/call target = %p.", jump_target);
        if (ud_obj.mnemonic == UD_Ijmp)
        {
          HADESMEM_DETAIL_TRACE_A("Writing resolved jump.");
          tramp_cur += detail::WriteJump(
            process_, tramp_cur, jump_target, true, &trampolines_);
        }
        else
        {
          HADESMEM_DETAIL_ASSERT(ud_obj.mnemonic == UD_Icall);
          HADESMEM_DETAIL_TRACE_A("Writing resolved call.");
          tramp_cur +=
            detail::WriteCall(process_, tramp_cur, jump_target, trampolines_);
        }
      }
      else
      {
        std::uint8_t const* const raw = ud_insn_ptr(&ud_obj);
        Write(process_, tramp_cur, raw, raw + len);
        tramp_cur += len;
      }

      instr_size += len;
    } while (instr_size < patch_size);

    HADESMEM_DETAIL_TRACE_A("Writing jump back to original code.");

    tramp_cur +=
      detail::WriteJump(process_,
                        tramp_cur,
                        reinterpret_cast<std::uint8_t*>(target_) + instr_size,
                        true,
                        &trampolines_);

    FlushInstructionCache(
      process_, trampoline_->GetBase(), trampoline_->GetSize());

    detail::WriteStubGate<TargetFuncT>(process_,
                                       stub_gate_->GetBase(),
                                       &*stub_,
                                       &GetOriginalArbitraryUserPtrPtr,
                                       &GetReturnAddressPtrPtr);

    orig_ = ReadVector<std::uint8_t>(process_, target_, patch_size);

    detail::VerifyPatchThreads(process_.GetId(), target_, orig_.size());

    WritePatch();

    FlushInstructionCache(process_, target_, instr_size);

    applied_ = true;
  }

  virtual void Remove() override
  {
    if (!applied_)
    {
      return;
    }

    SuspendedProcess const suspended_process{process_.GetId()};

    detail::VerifyPatchThreads(process_.GetId(), target_, orig_.size());
    detail::VerifyPatchThreads(
      process_.GetId(), trampoline_->GetBase(), trampoline_->GetSize());

    RemovePatch();

    // Don't free trampolines here. Do it in Apply/destructor. See comments in
    // Apply for the rationale.

    applied_ = false;
  }

  virtual void RemoveUnchecked() noexcept override
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

      applied_ = false;

      target_ = nullptr;
      detour_ = nullptr;

      trampoline_.reset();
      orig_.clear();
      trampolines_.clear();
    }
  }

  virtual void Detach() noexcept override
  {
    applied_ = false;

    detached_ = true;
  }

  virtual bool IsApplied() const noexcept override
  {
    return applied_;
  }

  virtual void* GetTrampoline() const noexcept override
  {
    return trampoline_->GetBase();
  }

  virtual std::atomic<std::uint32_t>& GetRefCount() override
  {
    return ref_count_;
  }

  virtual std::atomic<std::uint32_t> const& GetRefCount() const override
  {
    return ref_count_;
  }

  virtual bool CanHookChain() const noexcept override
  {
    return CanHookChainImpl();
  }

  virtual void* GetTarget() const noexcept override
  {
    return target_;
  }

  virtual void const* GetDetour() const noexcept override
  {
    return &detour_;
  }

  virtual void* GetContext() noexcept override
  {
    return &context_;
  }

  virtual void const* GetContext() const noexcept override
  {
    return &context_;
  }

protected:
  virtual std::size_t GetPatchSize() const
  {
    bool stub_near = detail::IsNear(target_, stub_gate_->GetBase());
    HADESMEM_DETAIL_TRACE_A(stub_near ? "Stub near." : "Stub far.");
    return stub_near ? detail::PatchConstants::kJmpSize32
                     : detail::PatchConstants::kJmpSize64;
  }

  virtual void WritePatch()
  {
    HADESMEM_DETAIL_TRACE_A("Writing jump to stub.");

    detail::WriteJump(
      process_, target_, stub_gate_->GetBase(), false, &trampolines_);
  }

  virtual void RemovePatch()
  {
    HADESMEM_DETAIL_TRACE_A("Restoring original bytes.");

    WriteVector(process_, target_, orig_);
  }

  virtual bool CanHookChainImpl() const noexcept
  {
    return true;
  }

  Process process_;
  bool applied_{false};
  bool detached_{false};
  void* target_{};
  DetourFuncT detour_{};
  std::unique_ptr<Allocator> trampoline_{};
  std::unique_ptr<Allocator> stub_gate_{};
  std::vector<BYTE> orig_{};
  std::vector<std::unique_ptr<Allocator>> trampolines_{};
  std::atomic<std::uint32_t> ref_count_{};
  std::unique_ptr<StubT> stub_{};
  ContextT context_;
};
}

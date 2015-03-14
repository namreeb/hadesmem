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
#include <hadesmem/patch_detour_base.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/thread.hpp>
#include <hadesmem/thread_list.hpp>
#include <hadesmem/thread_helpers.hpp>
#include <hadesmem/write.hpp>

namespace hadesmem
{
template <typename TargetFuncT> class PatchDetour : public PatchDetourBase
{
public:
  using TargetFuncRawT = std::add_pointer_t<std::remove_pointer_t<TargetFuncT>>;
  using StubT = detail::PatchDetourStub<TargetFuncT>;
  using DetourFuncRawT = typename StubT::DetourFuncRawT;
  using DetourFuncT = typename StubT::DetourFuncT;

  HADESMEM_DETAIL_STATIC_ASSERT(detail::IsFunction<TargetFuncT>::value);
  HADESMEM_DETAIL_STATIC_ASSERT(detail::IsFunction<TargetFuncRawT>::value);
  HADESMEM_DETAIL_STATIC_ASSERT(detail::IsFunction<DetourFuncRawT>::value);

  explicit PatchDetour(Process const& process,
                       TargetFuncT target,
                       DetourFuncT const& detour,
                       void* context = nullptr)
    : process_{&process},
      target_{target},
      detour_{detour},
      context_{context},
      stub_{std::make_unique<StubT>(this)}
  {
  }

  explicit PatchDetour(Process&& process,
                       TargetFuncT target,
                       DetourFuncT const& detour) = delete;

  PatchDetour(PatchDetour const& other) = delete;

  PatchDetour& operator=(PatchDetour const& other) = delete;

  PatchDetour(PatchDetour&& other)
    : process_{other.process_},
      applied_{other.applied_},
      target_{other.target_},
      detour_{std::move(other.detour_)},
      trampoline_{std::move(other.trampoline_)},
      stub_gate_{std::move(other.stub_gate_)},
      orig_(std::move(other.orig_)),
      trampolines_(std::move(other.trampolines_)),
      ref_count_{other.ref_count_.load()},
      stub_{other.stub_},
      orig_user_ptr_{other.orig_user_ptr_},
      context_{other.context_}
  {
    other.process_ = nullptr;
    other.applied_ = false;
    other.target_ = nullptr;
    other.stub_ = nullptr;
    other.orig_user_ptr_ = nullptr;
    other.context_ = nullptr;
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

    detour_ = std::move(other.detour_);

    trampoline_ = std::move(other.trampoline_);

    stub_gate_ = std::move(other.stub_gate_);

    orig_ = std::move(other.orig_);

    trampolines_ = std::move(other.trampolines_);

    ref_count_ = other.ref_count_.load();

    stub_ = other.stub_;
    other.stub_ = nullptr;

    orig_user_ptr_ = other.orig_user_ptr_;
    other.orig_user_ptr_ = nullptr;

    context_ = other.context_;
    other.context_ = nullptr;

    return *this;
  }

  virtual ~PatchDetour()
  {
    RemoveUnchecked();
  }

  virtual bool IsApplied() const HADESMEM_DETAIL_NOEXCEPT override
  {
    return applied_;
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

    SuspendedProcess const suspended_process{process_->GetId()};

    std::uint32_t const kMaxInstructionLen = 15;
    std::uint32_t const kTrampSize = kMaxInstructionLen * 3;

    trampoline_ = std::make_unique<Allocator>(*process_, kTrampSize);
    auto tramp_cur = static_cast<std::uint8_t*>(trampoline_->GetBase());

    auto const detour_raw = detour_.target<DetourFuncRawT>();
    if (detour_raw || detour_)
    {
      HADESMEM_DETAIL_TRACE_FORMAT_A(
        "Target = %p, Detour = %p, Trampoline = %p.",
        target_,
        detour_raw,
        trampoline_->GetBase());
    }
    else
    {
      HADESMEM_DETAIL_TRACE_FORMAT_A(
        "Target = %p, Detour = INVALID, Trampoline = %p.",
        target_,
        trampoline_->GetBase());
    }

    auto const buffer =
      ReadVector<std::uint8_t>(*process_, target_, kTrampSize);

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

    stub_gate_ = AllocatePageNear(target_);

    std::size_t const patch_size = GetPatchSize();

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
        std::int64_t const insn_target = [&]() -> std::int64_t
        {
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
          [](std::uint64_t base, std::int64_t target, std::uint32_t insn_len)
        {
          return reinterpret_cast<std::uint8_t*>(
                   static_cast<std::uintptr_t>(base)) +
                 target + insn_len;
        };

        std::uint64_t const insn_base = ud_insn_off(&ud_obj);
        std::uint32_t const insn_len = ud_insn_len(&ud_obj);

        auto const resolved_target =
          resolve_rel(insn_base, insn_target, insn_len);
        void* const jump_target =
          is_jimm ? resolved_target : Read<void*>(*process_, resolved_target);
        HADESMEM_DETAIL_TRACE_FORMAT_A("Jump/call target = %p.", jump_target);
        if (ud_obj.mnemonic == UD_Ijmp)
        {
          HADESMEM_DETAIL_TRACE_A("Writing resolved jump.");
          tramp_cur += WriteJump(tramp_cur, jump_target, true);
        }
        else
        {
          HADESMEM_DETAIL_ASSERT(ud_obj.mnemonic == UD_Icall);
          HADESMEM_DETAIL_TRACE_A("Writing resolved call.");
          tramp_cur += WriteCall(tramp_cur, jump_target);
        }
      }
      else
      {
        std::uint8_t const* const raw = ud_insn_ptr(&ud_obj);
        Write(*process_, tramp_cur, raw, raw + len);
        tramp_cur += len;
      }

      instr_size += len;
    } while (instr_size < patch_size);

    HADESMEM_DETAIL_TRACE_A("Writing jump back to original code.");

    tramp_cur += WriteJump(
      tramp_cur, reinterpret_cast<std::uint8_t*>(target_) + instr_size, true);

    FlushInstructionCache(
      *process_, trampoline_->GetBase(), trampoline_->GetSize());

    WriteStubGate(stub_gate_->GetBase());

    orig_ = ReadVector<std::uint8_t>(*process_, target_, patch_size);

    detail::VerifyPatchThreads(process_->GetId(), target_, orig_.size());

    WritePatch();

    FlushInstructionCache(*process_, target_, instr_size);

    applied_ = true;
  }

  virtual void Remove() override
  {
    if (!applied_)
    {
      return;
    }

    SuspendedProcess const suspended_process{process_->GetId()};

    detail::VerifyPatchThreads(process_->GetId(), target_, orig_.size());
    detail::VerifyPatchThreads(
      process_->GetId(), trampoline_->GetBase(), trampoline_->GetSize());

    RemovePatch();

    // Don't free trampolines here. Do it in Apply/destructor. See comments in
    // Apply for the rationale.

    applied_ = false;
  }

  void Detach()
  {
    applied_ = false;

    detached_ = true;
  }

  virtual PVOID GetTrampoline() const HADESMEM_DETAIL_NOEXCEPT override
  {
    return trampoline_->GetBase();
  }

  // Ref count is user-managed and only here for convenience purposes.
  virtual std::atomic<std::uint32_t>& GetRefCount() override
  {
    return ref_count_;
  }

  virtual std::atomic<std::uint32_t> const& GetRefCount() const override
  {
    return ref_count_;
  }

  virtual bool CanHookChain() const override
  {
    return CanHookChainImpl();
  }

  virtual void const* GetDetour() const HADESMEM_DETAIL_NOEXCEPT override
  {
    return &detour_;
  }

  virtual void* GetContext() const HADESMEM_DETAIL_NOEXCEPT override
  {
    return context_;
  }

  virtual void*
    GetOriginalArbitraryUserPtr() const HADESMEM_DETAIL_NOEXCEPT override
  {
    return orig_user_ptr_;
  }

protected:
  virtual std::size_t GetPatchSize() const
  {
    bool stub_near = IsNear(target_, stub_gate_->GetBase());
    HADESMEM_DETAIL_TRACE_A(stub_near ? "Stub near." : "Stub far.");
    return stub_near ? kJmpSize32 : kJmpSize64;
  }

  virtual void WritePatch()
  {
    HADESMEM_DETAIL_TRACE_A("Writing jump to stub.");

    WriteJump(target_, stub_gate_->GetBase(), false);
  }

  virtual void RemovePatch()
  {
    HADESMEM_DETAIL_TRACE_A("Restoring original bytes.");

    WriteVector(*process_, target_, orig_);
  }

  virtual bool CanHookChainImpl() const
  {
    return true;
  }

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
  std::unique_ptr<Allocator> AllocatePageNear(PVOID address)
  {
    SYSTEM_INFO sys_info{};
    GetSystemInfo(&sys_info);
    DWORD const page_size = sys_info.dwPageSize;

#if defined(HADESMEM_DETAIL_ARCH_X64)
    std::intptr_t const search_beg = (std::max)(
      reinterpret_cast<std::intptr_t>(address) - 0x7FFFFF00LL,
      reinterpret_cast<std::intptr_t>(sys_info.lpMinimumApplicationAddress));
    std::intptr_t const search_end = (std::min)(
      reinterpret_cast<std::intptr_t>(address) + 0x7FFFFF00LL,
      reinterpret_cast<std::intptr_t>(sys_info.lpMaximumApplicationAddress));

    std::unique_ptr<Allocator> trampoline;

    auto const allocate_tramp =
      [](Process const& process, PVOID addr, SIZE_T size)
        -> std::unique_ptr<Allocator>
    {
      auto const new_addr = detail::TryAlloc(process, size, addr);
      return new_addr
               ? std::make_unique<Allocator>(process, size, new_addr, true)
               : std::unique_ptr<Allocator>();
    };

    // Do two separate passes when looking for trampolines, ensuring to scan
    // forwards first. This is because there is a bug in Steam's overlay (last
    // checked and confirmed in SteamOverlayRender64.dll v2.50.25.37) where
    // negative displacements are not correctly sign-extended when cast to
    // 64-bits, resulting in a crash when they attempt to resolve the jump.

    // .text:0000000180082956                 cmp     al, 0FFh
    // .text:0000000180082958                 jnz     short loc_180082971
    // .text:000000018008295A                 cmp     byte ptr [r13+1], 25h
    // .text:000000018008295F                 jnz     short loc_180082971
    // ; Notice how the displacement is not being sign extended.
    // .text:0000000180082961                 mov     eax, [r13+2]
    // .text:0000000180082965                 lea     rcx, [rax+r13]
    // .text:0000000180082969                 mov     r13, [rcx+6]

    for (std::intptr_t base = reinterpret_cast<std::intptr_t>(address),
                       index = 0;
         base + index < search_end && !trampoline;
         index += page_size)
    {
      trampoline = allocate_tramp(
        *process_, reinterpret_cast<void*>(base + index), page_size);
    }

    if (!trampoline)
    {
      HADESMEM_DETAIL_TRACE_A(
        "WARNING! Failed to find a viable trampoline "
        "page in forward scan, falling back to backward scan. This may cause "
        "incompatibilty with some other overlays.");
    }

    for (std::intptr_t base = reinterpret_cast<std::intptr_t>(address),
                       index = 0;
         base - index > search_beg && !trampoline;
         index += page_size)
    {
      trampoline = allocate_tramp(
        *process_, reinterpret_cast<void*>(base - index), page_size);
    }

    if (!trampoline)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error{} << ErrorString{"Failed to find trampoline memory block."});
    }

    return trampoline;
#elif defined(HADESMEM_DETAIL_ARCH_X86)
    (void)address;
    return std::make_unique<Allocator>(*process_, page_size);
#else
#error "[HadesMem] Unsupported architecture."
#endif
  }

  bool IsNear(void* address, void* target) const HADESMEM_DETAIL_NOEXCEPT
  {
#if defined(HADESMEM_DETAIL_ARCH_X64)
    auto const rel = reinterpret_cast<std::intptr_t>(target) -
                     reinterpret_cast<std::intptr_t>(address) - 5;
    return rel > (std::numeric_limits<std::uint32_t>::min)() &&
           rel < (std::numeric_limits<std::uint32_t>::max)();
#elif defined(HADESMEM_DETAIL_ARCH_X86)
    (void)address;
    (void)target;
    return true;
#else
#error "[HadesMem] Unsupported architecture."
#endif
  }

  std::vector<std::uint8_t> GenJmp32(void* address, void* target) const
  {
    std::vector<std::uint8_t> buf = {0xE9, 0xEB, 0xBE, 0xAD, 0xDE};
    auto const dst_len = sizeof(std::uint32_t);
    auto const op_len = 1;
    auto const disp = reinterpret_cast<std::uintptr_t>(target) -
                      reinterpret_cast<std::uintptr_t>(address) - dst_len -
                      op_len;
    *reinterpret_cast<std::uint32_t*>(&buf[op_len]) =
      static_cast<std::uint32_t>(disp);
    return buf;
  }

  std::vector<std::uint8_t> GenCall32(void* address, void* target) const
  {
    std::vector<std::uint8_t> buf = {0xE8, 0xEB, 0xBE, 0xAD, 0xDE};
    auto const dst_len = sizeof(std::uint32_t);
    auto const op_len = 1;
    auto const disp = reinterpret_cast<std::uintptr_t>(target) -
                      reinterpret_cast<std::uintptr_t>(address) - dst_len -
                      op_len;
    *reinterpret_cast<std::uint32_t*>(&buf[op_len]) =
      static_cast<std::uint32_t>(disp);
    return buf;
  }

  std::vector<std::uint8_t> GenJmpTramp64(void* address, void* target) const
  {
    std::vector<std::uint8_t> buf = {0xFF, 0x25, 0xEF, 0xBE, 0xAD, 0xDE};
    auto const dst_len = sizeof(std::uint32_t);
    auto const op_len = 2;
    auto const disp = reinterpret_cast<std::uintptr_t>(target) -
                      reinterpret_cast<std::uintptr_t>(address) - dst_len -
                      op_len;
    *reinterpret_cast<std::uint32_t*>(&buf[op_len]) =
      static_cast<std::uint32_t>(disp);
    return buf;
  }

  std::vector<std::uint8_t> GenCallTramp64(void* address, void* target) const
  {
    std::vector<std::uint8_t> buf = {0xFF, 0x15, 0xEF, 0xBE, 0xAD, 0xDE};
    auto const dst_len = sizeof(std::uint32_t);
    auto const op_len = 2;
    auto const disp = reinterpret_cast<std::uintptr_t>(target) -
                      reinterpret_cast<std::uintptr_t>(address) - dst_len -
                      op_len;
    *reinterpret_cast<std::uint32_t*>(&buf[op_len]) =
      static_cast<std::uint32_t>(disp);
    return buf;
  }

  std::vector<std::uint8_t> GenPush32Ret(void* target) const
  {
    std::vector<std::uint8_t> buf = {// PUSH 0xDEADBEEF
                                     0x68,
                                     0xEF,
                                     0xBE,
                                     0xAD,
                                     0xDE,
                                     // RET
                                     0xC3};
    auto const op_len = 1;
    auto const target_low = static_cast<std::uint32_t>(
      reinterpret_cast<std::uintptr_t>(target) & 0xFFFFFFFF);
    *reinterpret_cast<std::uint32_t*>(&buf[op_len]) = target_low;
    return buf;
  }

  std::vector<std::uint8_t> GenPush64Ret(void* target) const
  {
    std::vector<std::uint8_t> buf = {// PUSH 0xDEADBEEF
                                     0x68,
                                     0xEF,
                                     0xBE,
                                     0xAD,
                                     0xDE,
                                     // MOV DWORD PTR [RSP+0x4], 0xDEADBEEF
                                     0xC7,
                                     0x44,
                                     0x24,
                                     0x04,
                                     0xEF,
                                     0xBE,
                                     0xAD,
                                     0xDE,
                                     // RET
                                     0xC3};
    auto const low_data_offs = 1;
    auto const high_data_offs = 9;
    auto const target_uint = reinterpret_cast<std::uint64_t>(target);
    auto const target_high =
      static_cast<std::uint32_t>((target_uint >> 32) & 0xFFFFFFFF);
    auto const target_low =
      static_cast<std::uint32_t>(target_uint & 0xFFFFFFFF);
    *reinterpret_cast<std::uint32_t*>(&buf[low_data_offs]) = target_low;
    *reinterpret_cast<std::uint32_t*>(&buf[high_data_offs]) = target_high;
    return buf;
  }

  std::size_t WriteJump(void* address, void* target, bool push_ret_fallback)
  {
    (void)push_ret_fallback;
    HADESMEM_DETAIL_TRACE_FORMAT_A(
      "Address = %p, Target = %p, Push Ret Fallback = %u.",
      address,
      target,
      static_cast<std::uint32_t>(push_ret_fallback));

    std::vector<std::uint8_t> jump_buf;

#if defined(HADESMEM_DETAIL_ARCH_X64)
    if (IsNear(address, target))
    {
      HADESMEM_DETAIL_TRACE_A("Using relative jump.");
      jump_buf = GenJmp32(address, target);
      HADESMEM_DETAIL_ASSERT(jump_buf.size() == kJmpSize32);
    }
    else
    {
      std::unique_ptr<Allocator> trampoline;
      try
      {
        trampoline = AllocatePageNear(address);
      }
      catch (std::exception const& /*e*/)
      {
        // Don't need to do anything, we'll fall back to PUSH/RET.
      }

      if (trampoline)
      {
        void* tramp_addr = trampoline->GetBase();

        HADESMEM_DETAIL_TRACE_FORMAT_A(
          "Using trampoline jump. Trampoline = %p.", tramp_addr);

        Write(*process_, tramp_addr, target);

        trampolines_.emplace_back(std::move(trampoline));

        jump_buf = GenJmpTramp64(address, tramp_addr);
        HADESMEM_DETAIL_ASSERT(jump_buf.size() == kJmpSize64);
      }
      else
      {
        if (!push_ret_fallback)
        {
          // We're out of options...
          HADESMEM_DETAIL_THROW_EXCEPTION(
            Error{} << ErrorString{"Unable to use a relative or trampoline "
                                   "jump, and push/ret fallback is disabled."});
        }

        HADESMEM_DETAIL_TRACE_A("Using push/ret 'jump'.");

        auto const target_high = static_cast<std::uint32_t>(
          (reinterpret_cast<std::uintptr_t>(target) >> 32) & 0xFFFFFFFF);
        if (target_high)
        {
          HADESMEM_DETAIL_TRACE_A("Push/ret 'jump' is big.");
          jump_buf = GenPush64Ret(target);
          HADESMEM_DETAIL_ASSERT(jump_buf.size() == kPushRetSize64);
        }
        else
        {
          HADESMEM_DETAIL_TRACE_A("Push/ret 'jump' is small.");
          jump_buf = GenPush32Ret(target);
          HADESMEM_DETAIL_ASSERT(jump_buf.size() == kPushRetSize32);
        }
      }
    }
#elif defined(HADESMEM_DETAIL_ARCH_X86)
    HADESMEM_DETAIL_TRACE_A("Using relative jump.");
    jump_buf = GenJmp32(address, target);
    HADESMEM_DETAIL_ASSERT(jump_buf.size() == kJmpSize32);
#else
#error "[HadesMem] Unsupported architecture."
#endif

    WriteVector(*process_, address, jump_buf);

    return jump_buf.size();
  }

  std::size_t WriteCall(void* address, void* target)
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A(
      "Address = %p, Target = %p", address, target);

    std::vector<std::uint8_t> call_buf;

#if defined(HADESMEM_DETAIL_ARCH_X64)
    std::unique_ptr<Allocator> trampoline = AllocatePageNear(address);

    PVOID tramp_addr = trampoline->GetBase();

    HADESMEM_DETAIL_TRACE_FORMAT_A("Using trampoline call. Trampoline = %p.",
                                   tramp_addr);

    Write(*process_, tramp_addr, reinterpret_cast<std::uintptr_t>(target));

    trampolines_.emplace_back(std::move(trampoline));

    call_buf = GenCallTramp64(address, tramp_addr);
    HADESMEM_DETAIL_ASSERT(call_buf.size() == kCallSize64);
#elif defined(HADESMEM_DETAIL_ARCH_X86)
    HADESMEM_DETAIL_TRACE_A("Using relative call.");
    call_buf = GenCall32(address, target);
    HADESMEM_DETAIL_ASSERT(call_buf.size() == kCallSize32);
#else
#error "[HadesMem] Unsupported architecture."
#endif

    WriteVector(*process_, address, call_buf);

    return call_buf.size();
  }

  std::vector<std::uint8_t> GenStubGate32()
  {
    std::vector<std::uint8_t> buf = {// PUSH EAX
                                     0x50,
                                     // PUSH EDX
                                     0x52,
                                     // MOV EAX, FS:[0x14]
                                     0x64,
                                     0xA1,
                                     0x14,
                                     0x00,
                                     0x00,
                                     0x00,
                                     // MOV EDX, 0xDEADBEEF
                                     0xBA,
                                     0xEF,
                                     0xBE,
                                     0xAD,
                                     0xDE,
                                     // MOV FS:[0x14], EDX
                                     0x64,
                                     0x89,
                                     0x15,
                                     0x14,
                                     0x00,
                                     0x00,
                                     0x00,
                                     // MOV EDX, 0xCAFEBABE
                                     0xBA,
                                     0xBE,
                                     0xBA,
                                     0xFE,
                                     0xCA,
                                     // MOV [EDX], EAX
                                     0x89,
                                     0x02,
                                     // POP EDX
                                     0x5A,
                                     // POP EAX
                                     0x58};
    std::size_t const kStubPtrOfs = 9;
    std::size_t const kUserPtrOfs = 21;
    HADESMEM_DETAIL_ASSERT(stub_);
    *reinterpret_cast<void**>(&buf[kStubPtrOfs]) = &*stub_;
    *reinterpret_cast<void**>(&buf[kUserPtrOfs]) = &orig_user_ptr_;
    return buf;
  }

  std::vector<std::uint8_t> GenStubGate64()
  {
    std::vector<std::uint8_t> buf = {
      // PUSH RAX
      0x50,
      // PUSH RDX
      0x52,
      // MOV RAX, GS:[0x28]
      0x65, 0x48, 0x8B, 0x04, 0x25, 0x28, 0x00, 0x00, 0x00,
      // MOV RDX, 0xDEADBEEFDEADBEEF
      0x48, 0xBA, 0xEF, 0xBE, 0xAD, 0xDE, 0xEF, 0xBE, 0xAD, 0xDE,
      // MOV GS:[0x28], RDX
      0x65, 0x48, 0x89, 0x14, 0x25, 0x28, 0x00, 0x00, 0x00,
      // MOV RDX, 0xCAFEBABECAFEBABE
      0x48, 0xBA, 0xBE, 0xBA, 0xFE, 0xCA, 0xBE, 0xBA, 0xFE, 0xCA,
      // MOV[RDX], RAX
      0x48, 0x89, 0x02,
      // POP RDX
      0x5A,
      // POP RAX
      0x58};
    std::size_t const kStubPtrOfs = 13;
    std::size_t const kUserPtrOfs = 32;
    HADESMEM_DETAIL_ASSERT(stub_);
    *reinterpret_cast<void**>(&buf[kStubPtrOfs]) = &*stub_;
    *reinterpret_cast<void**>(&buf[kUserPtrOfs]) = &orig_user_ptr_;
    return buf;
  }

  void WriteStubGate(void* address)
  {
#if defined(HADESMEM_DETAIL_ARCH_X64)
    auto const stub_gate = GenStubGate64();
#elif defined(HADESMEM_DETAIL_ARCH_X86)
    auto const stub_gate = GenStubGate32();
#else
#error "[HadesMem] Unsupported architecture."
#endif
    WriteVector(*process_, address, stub_gate);
    WriteJump(static_cast<std::uint8_t*>(address) + stub_gate.size(),
              &StubT::Stub,
              true);
  }

  static std::size_t const kJmpSize32 = 5;
  static std::size_t const kCallSize32 = 5;
#if defined(HADESMEM_DETAIL_ARCH_X64)
  static std::size_t const kJmpSize64 = 6;
  static std::size_t const kCallSize64 = 6;
  static std::size_t const kPushRetSize64 = 14;
  static std::size_t const kPushRetSize32 = 6;
#elif defined(HADESMEM_DETAIL_ARCH_X86)
  static std::size_t const kJmpSize64 = kJmpSize32;
  static std::size_t const kCallSize64 = kCallSize32;
#else
#error "[HadesMem] Unsupported architecture."
#endif

  Process const* process_;
  bool applied_{false};
  bool detached_{false};
  TargetFuncRawT target_;
  DetourFuncT detour_;
  std::unique_ptr<Allocator> trampoline_;
  std::unique_ptr<Allocator> stub_gate_;
  std::vector<BYTE> orig_;
  std::vector<std::unique_ptr<Allocator>> trampolines_;
  std::atomic<std::uint32_t> ref_count_;
  std::unique_ptr<StubT> stub_;
  void* orig_user_ptr_{nullptr};
  void* context_{nullptr};
};
}

// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include <windows.h>

#include <hadesmem/alloc.hpp>
#include <hadesmem/detail/alias_cast.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/patch_detour_stub.hpp>
#include <hadesmem/detail/scope_warden.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/detail/type_traits.hpp>
#include <hadesmem/detail/winternl.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/flush.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

namespace hadesmem
{
namespace detail
{
struct PatchConstants
{
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
};

// Inspired by EasyHook.
inline std::unique_ptr<Allocator> AllocatePageNear(Process const& process,
                                                   void* address)
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

  auto const allocate_tramp = [](Process const& process,
                                 PVOID addr,
                                 SIZE_T size) -> std::unique_ptr<Allocator> {
    auto const new_addr = detail::TryAlloc(process, size, addr);
    return new_addr ? std::make_unique<Allocator>(process, size, new_addr, true)
                    : std::unique_ptr<Allocator>();
  };

  // NOTE: The issue described below now appears to be fixed (the mov is now a
  // movsxd), but it doesn't hurt to keep the logic this way (especially since
  // it's a fairly generic problem that other hooking libraries are likely to
  // have at some point or another).
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

  for (std::intptr_t base = reinterpret_cast<std::intptr_t>(address), index = 0;
       base + index < search_end && !trampoline;
       index += page_size)
  {
    trampoline =
      allocate_tramp(process, reinterpret_cast<void*>(base + index), page_size);
  }

  if (!trampoline)
  {
    HADESMEM_DETAIL_TRACE_A(
      "WARNING! Failed to find a viable trampoline "
      "page in forward scan, falling back to backward scan. This may cause "
      "incompatibilty with some other overlays.");
  }

  for (std::intptr_t base = reinterpret_cast<std::intptr_t>(address), index = 0;
       base - index > search_beg && !trampoline;
       index += page_size)
  {
    trampoline =
      allocate_tramp(process, reinterpret_cast<void*>(base - index), page_size);
  }

  if (!trampoline)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error{} << ErrorString{"Failed to find trampoline memory block."});
  }

  return trampoline;
#elif defined(HADESMEM_DETAIL_ARCH_X86)
  (void)address;
  return std::make_unique<Allocator>(process, page_size);
#else
#error "[HadesMem] Unsupported architecture."
#endif
}

inline bool IsNear(void* address, void* target) noexcept
{
#if defined(HADESMEM_DETAIL_ARCH_X64)
  auto const rel = reinterpret_cast<std::intptr_t>(target) -
                   reinterpret_cast<std::intptr_t>(address) - 5;
  return rel > (std::numeric_limits<std::int32_t>::min)() &&
         rel < (std::numeric_limits<std::int32_t>::max)();
#elif defined(HADESMEM_DETAIL_ARCH_X86)
  (void)address;
  (void)target;
  return true;
#else
#error "[HadesMem] Unsupported architecture."
#endif
}

inline std::vector<std::uint8_t> GenJmp32(void* address, void* target)
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

inline std::vector<std::uint8_t> GenCall32(void* address, void* target)
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

inline std::vector<std::uint8_t> GenJmpTramp64(void* address, void* target)
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

inline std::vector<std::uint8_t> GenCallTramp64(void* address, void* target)
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

inline std::vector<std::uint8_t> GenPush32Ret(void* target)
{
  // clang-format off
  std::vector<std::uint8_t> buf =
  {
    // PUSH 0xDEADBEEF
    0x68, 0xEF, 0xBE, 0xAD, 0xDE,
    // RET
    0xC3
  };
  // clang-format on
  auto const op_len = 1;
  auto const target_low = static_cast<std::uint32_t>(
    reinterpret_cast<std::uintptr_t>(target) & 0xFFFFFFFF);
  *reinterpret_cast<std::uint32_t*>(&buf[op_len]) = target_low;
  return buf;
}

inline std::vector<std::uint8_t> GenPush64Ret(void* target)
{
  // clang-format off
  std::vector<std::uint8_t> buf =
  {
    // PUSH 0xDEADBEEF
    0x68, 0xEF, 0xBE, 0xAD, 0xDE,
    // MOV DWORD PTR [RSP+0x4], 0xDEADBEEF
    0xC7, 0x44, 0x24, 0x04, 0xEF, 0xBE, 0xAD, 0xDE,
    // RET
    0xC3
  };
  // clang-format on
  auto const low_data_offs = 1;
  auto const high_data_offs = 9;
  auto const target_uint = reinterpret_cast<std::uint64_t>(target);
  auto const target_high =
    static_cast<std::uint32_t>((target_uint >> 32) & 0xFFFFFFFF);
  auto const target_low = static_cast<std::uint32_t>(target_uint & 0xFFFFFFFF);
  *reinterpret_cast<std::uint32_t*>(&buf[low_data_offs]) = target_low;
  *reinterpret_cast<std::uint32_t*>(&buf[high_data_offs]) = target_high;
  return buf;
}

inline std::size_t
  WriteJump(Process const& process,
            void* address,
            void* target,
            bool push_ret_fallback,
            std::vector<std::unique_ptr<Allocator>>* trampolines)
{
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
    HADESMEM_DETAIL_ASSERT(jump_buf.size() == PatchConstants::kJmpSize32);
  }
  else
  {
    std::unique_ptr<Allocator> trampoline;

    if (trampolines)
    {
      try
      {
        trampoline = AllocatePageNear(process, address);
      }
      catch (std::exception const& /*e*/)
      {
        // Don't need to do anything, we'll fall back to PUSH/RET.
      }
    }

    if (trampoline)
    {
      void* tramp_addr = trampoline->GetBase();

      HADESMEM_DETAIL_TRACE_FORMAT_A("Using trampoline jump. Trampoline = %p.",
                                     tramp_addr);

      Write(process, tramp_addr, target);

      trampolines->emplace_back(std::move(trampoline));

      jump_buf = GenJmpTramp64(address, tramp_addr);
      HADESMEM_DETAIL_ASSERT(jump_buf.size() == PatchConstants::kJmpSize64);
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
        HADESMEM_DETAIL_ASSERT(jump_buf.size() ==
                               PatchConstants::kPushRetSize64);
      }
      else
      {
        HADESMEM_DETAIL_TRACE_A("Push/ret 'jump' is small.");
        jump_buf = GenPush32Ret(target);
        HADESMEM_DETAIL_ASSERT(jump_buf.size() ==
                               PatchConstants::kPushRetSize32);
      }
    }
  }
#elif defined(HADESMEM_DETAIL_ARCH_X86)
  (void)push_ret_fallback;
  (void)trampolines;
  HADESMEM_DETAIL_TRACE_A("Using relative jump.");
  jump_buf = GenJmp32(address, target);
  HADESMEM_DETAIL_ASSERT(jump_buf.size() == PatchConstants::kJmpSize32);
#else
#error "[HadesMem] Unsupported architecture."
#endif

  WriteVector(process, address, jump_buf);

  return jump_buf.size();
}

inline std::size_t
  WriteCall(Process const& process,
            void* address,
            void* target,
            std::vector<std::unique_ptr<Allocator>>& trampolines)
{
  HADESMEM_DETAIL_TRACE_FORMAT_A("Address = %p, Target = %p", address, target);

  std::vector<std::uint8_t> call_buf;

// TODO: Avoid using a trampoline where possible.
#if defined(HADESMEM_DETAIL_ARCH_X64)
  std::unique_ptr<Allocator> trampoline = AllocatePageNear(process, address);

  PVOID tramp_addr = trampoline->GetBase();

  HADESMEM_DETAIL_TRACE_FORMAT_A("Using trampoline call. Trampoline = %p.",
                                 tramp_addr);

  Write(process, tramp_addr, reinterpret_cast<std::uintptr_t>(target));

  trampolines.emplace_back(std::move(trampoline));

  call_buf = GenCallTramp64(address, tramp_addr);
  HADESMEM_DETAIL_ASSERT(call_buf.size() == PatchConstants::kCallSize64);
#elif defined(HADESMEM_DETAIL_ARCH_X86)
  (void)trampolines;
  HADESMEM_DETAIL_TRACE_A("Using relative call.");
  call_buf = GenCall32(address, target);
  HADESMEM_DETAIL_ASSERT(call_buf.size() == PatchConstants::kCallSize32);
#else
#error "[HadesMem] Unsupported architecture."
#endif

  WriteVector(process, address, call_buf);

  return call_buf.size();
}

// TODO: Add frame pointer so we can unwind the stack while debugging?
// TODO: Ensure we're correcty saving all registers/state. Currently
// we're only saving regular registers. What about eflags, fpu, sse, etc.
inline std::vector<std::uint8_t> GenStubGate32(void* stub,
                                               void* get_orig_user_ptr_ptr_fn,
                                               void* get_ret_address_ptr_ptr_fn)
{
  HADESMEM_DETAIL_ASSERT(stub);
  HADESMEM_DETAIL_ASSERT(get_orig_user_ptr_ptr_fn);
  // clang-format off
  std::vector<std::uint8_t> buf =
  {
    // Add NOPs so Steam overlay works. It follows our hook, and it does not 
    // recognize the opcode sequence otherwise.
    // NOP (x 5)
    0x90, 0x90, 0x90, 0x90, 0x90,
    // PUSHAD
    0x60,
    // MOV ECX, DWORD PTR SS:[ESP+0x20]
    0x8B, 0x4C, 0x24, 0x20,
    // MOV EAX, 0xDEADBABE
    0xB8, 0xBE, 0xBA, 0xAD, 0xDE,
    // PUSH ECX
    0x51,
    // CALL EAX
    0xFF, 0xD0,
    // POP ECX,
    0x59,
    // MOV [EAX], ECX
    0x89, 0x08,
    // MOV ECX, FS:[0x14]
    0x64, 0x8B, 0x0D, 0x14, 0x00, 0x00, 0x00,
    // MOV EAX, 0xDEADBEEF
    0xB8, 0xEF, 0xBE, 0xAD, 0xDE,
    // MOV FS:[0x14], EAX
    0x64, 0xA3, 0x14, 0x00, 0x00, 0x00,
    // MOV EAX, 0xCAFEBABE
    0xB8, 0xBE, 0xBA, 0xFE, 0xCA,
    // PUSH ECX
    0x51,
    // CALL EAX
    0xFF, 0xD0,
    // POP ECX
    0x59,
    // MOV [EAX], ECX
    0x89, 0x08,
    // POPAD
    0x61
  };
  // clang-format on
  std::size_t const kRetAddrPtrOfs = 11;
  std::size_t const kStubPtrOfs = 29;
  std::size_t const kUserPtrOfs = 40;
  *reinterpret_cast<void**>(&buf[kRetAddrPtrOfs]) = get_ret_address_ptr_ptr_fn;
  *reinterpret_cast<void**>(&buf[kStubPtrOfs]) = stub;
  *reinterpret_cast<void**>(&buf[kUserPtrOfs]) = get_orig_user_ptr_ptr_fn;
  return buf;
}

// TODO: Ensure we're correcty saving all registers/state. Currently
// we're only saving regular registers. What about eflags, fpu, sse, etc.
inline std::vector<std::uint8_t> GenStubGate64(void* stub,
                                               void* get_orig_user_ptr_ptr_fn,
                                               void* get_ret_address_ptr_ptr_fn)
{
  HADESMEM_DETAIL_ASSERT(stub);
  HADESMEM_DETAIL_ASSERT(get_orig_user_ptr_ptr_fn);
  // clang-format off
  std::vector<std::uint8_t> buf =
  {
    // PUSH RAX
    0x50,
    // PUSH RCX
    0x51,
    // PUSH RDX
    0x52,
    // PUSH RBX
    0x53,
    // PUSH RBP
    0x55,
    // PUSH RSI
    0x56,
    // PUSH RDI
    0x57,
    // PUSH R8
    0x41, 0x50,
    // PUSH R9
    0x41, 0x51,
    // PUSH R10
    0x41, 0x52,
    // PUSH R11
    0x41, 0x53,
    // PUSH R12
    0x41, 0x54,
    // PUSH R13
    0x41, 0x55,
    // PUSH R14
    0x41, 0x56,
    // PUSH R15
    0x41, 0x57,
    // MOV RCX, QWORD PTR SS:[RSP+0x78]
    0x48, 0x8B, 0x4C, 0x24, 0x78,
    // MOV RAX, 0xDEADBABEDEADBABE
    0x48, 0xB8, 0xBE, 0xBA, 0xAD, 0xDE, 0xBE, 0xBA, 0xAD, 0xDE,
    // PUSH RCX
    0x51,
    // CALL RAX
    0xFF, 0xD0,
    // POP RCX
    0x59,
    // MOV [RAX], ECX
    0x48, 0x89, 0x08,
    // MOV RCX, GS:[0x28]
    0x65, 0x48, 0x8B, 0x0C, 0x25, 0x28, 0x00, 0x00, 0x00,
    // MOV RAX, 0xDEADBEEFDEADBEEF
    0x48, 0xB8, 0xEF, 0xBE, 0xAD, 0xDE, 0xEF, 0xBE, 0xAD, 0xDE,
    // MOV GS:[0x28], RAX
    0x65, 0x48, 0x89, 0x04, 0x25, 0x28, 0x00, 0x00, 0x00,
    // MOV RAX, 0xCAFEBABECAFEBABE
    0x48, 0xB8, 0xBE, 0xBA, 0xFE, 0xCA, 0xBE, 0xBA, 0xFE, 0xCA,
    // PUSH RCX
    0x51,
    // CALL RAX
    0xFF, 0xD0,
    // POP RCX
    0x59,
    // MOV [RAX], RCX
    0x48, 0x89, 0x08,
    // POP R15
    0x41, 0x5F,
    // POP R14
    0x41, 0x5E,
    // POP R13
    0x41, 0x5D,
    // POP R12
    0x41, 0x5C,
    // POP R11
    0x41, 0x5B,
    // POP R10
    0x41, 0x5A,
    // POP R9
    0x41, 0x59,
    // POP R8
    0x41, 0x58,
    // POP RDI
    0x5F,
    // POP RSI
    0x5E,
    // POP RBP
    0x5D,
    // POP RBX
    0x5B,
    // POP RDX
    0x5A,
    // POP RCX
    0x59,
    // POP RAX
    0x58
  };
  // clang-format on
  std::size_t const kRetAddrPtrOfs = 30;
  std::size_t const kStubPtrOfs = 56;
  std::size_t const kUserPtrOfs = 75;
  *reinterpret_cast<void**>(&buf[kRetAddrPtrOfs]) = get_ret_address_ptr_ptr_fn;
  *reinterpret_cast<void**>(&buf[kStubPtrOfs]) = stub;
  *reinterpret_cast<void**>(&buf[kUserPtrOfs]) = get_orig_user_ptr_ptr_fn;
  return buf;
}

template <typename TargetFuncT>
inline void WriteStubGate(Process const& process,
                          void* address,
                          void* stub,
                          void* get_orig_user_ptr_ptr_fn,
                          void* get_ret_address_ptr_ptr_fn)
{
  using StubT = typename PatchDetourStub<TargetFuncT>;
#if defined(HADESMEM_DETAIL_ARCH_X64)
  auto const stub_gate =
    GenStubGate64(stub, get_orig_user_ptr_ptr_fn, get_ret_address_ptr_ptr_fn);
#elif defined(HADESMEM_DETAIL_ARCH_X86)
  auto const stub_gate =
    GenStubGate32(stub, get_orig_user_ptr_ptr_fn, get_ret_address_ptr_ptr_fn);
#else
#error "[HadesMem] Unsupported architecture."
#endif
  WriteVector(process, address, stub_gate);
  WriteJump(process,
            static_cast<std::uint8_t*>(address) + stub_gate.size(),
            &StubT::Stub,
            true,
            nullptr);
  FlushInstructionCache(process, address, stub_gate.size());
}
}
}
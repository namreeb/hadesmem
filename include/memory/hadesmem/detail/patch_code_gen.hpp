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

  auto const allocate_tramp =
    [](Process const& process, PVOID addr, SIZE_T size)
      -> std::unique_ptr<Allocator>
  {
    auto const new_addr = detail::TryAlloc(process, size, addr);
    return new_addr ? std::make_unique<Allocator>(process, size, new_addr, true)
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

inline bool IsNear(void* address, void* target) HADESMEM_DETAIL_NOEXCEPT
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

inline std::vector<std::uint8_t> GenPush64Ret(void* target)
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

inline std::vector<std::uint8_t> GenStubGate32(void* stub,
                                               void** orig_user_ptr_ptr)
{
  HADESMEM_DETAIL_ASSERT(stub);
  HADESMEM_DETAIL_ASSERT(orig_user_ptr_ptr);
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
  *reinterpret_cast<void**>(&buf[kStubPtrOfs]) = stub;
  *reinterpret_cast<void**>(&buf[kUserPtrOfs]) = orig_user_ptr_ptr;
  return buf;
}

inline std::vector<std::uint8_t> GenStubGate64(void* stub,
                                               void** orig_user_ptr_ptr)
{
  HADESMEM_DETAIL_ASSERT(stub);
  HADESMEM_DETAIL_ASSERT(orig_user_ptr_ptr);
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
  *reinterpret_cast<void**>(&buf[kStubPtrOfs]) = stub;
  *reinterpret_cast<void**>(&buf[kUserPtrOfs]) = orig_user_ptr_ptr;
  return buf;
}

template <typename TargetFuncT>
inline void WriteStubGate(Process const& process,
                          void* address,
                          void* stub,
                          void** orig_user_ptr_ptr)
{
  using StubT = typename PatchDetourStub<TargetFuncT>;
#if defined(HADESMEM_DETAIL_ARCH_X64)
  auto const stub_gate = GenStubGate64(stub, orig_user_ptr_ptr);
#elif defined(HADESMEM_DETAIL_ARCH_X86)
  auto const stub_gate = GenStubGate32(stub, orig_user_ptr_ptr);
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

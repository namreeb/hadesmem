// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/find_pattern.hpp>
#include <hadesmem/detail/winternl.hpp>
#include <hadesmem/detail/trace.hpp>

// It would be better to use data structures already available
// from 3rd part libraries that hadesmem already includes in order
// to provide the x86 instruction encodings.

namespace hadesmem
{
namespace detail
{
#pragma pack(push, 1)
struct x86AddInstruction
{
  std::int16_t Opcode;
  std::int32_t Operand;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct x64LeaInstruction
{
  std::uint8_t SecondaryOpcode;
  std::uint8_t PrimaryOpcode;
  std::uint8_t Register;
  std::uint32_t Operand;
};
#pragma pack(pop)

inline winternl::PVECTORED_HANDLER_ENTRY
  GetVectoredEhPointer(hadesmem::Process const& process)
{
#if defined(HADESMEM_DETAIL_ARCH_X86)
  using InstructionPtrT = x86AddInstruction;
  std::wstring const kInstructionPattern{L"81 C3 ?? ?? ?? ?? 53 89 46 0C"};
#elif defined(HADESMEM_DETAIL_ARCH_X64)
  using InstructionPtrT = x64LeaInstruction;
  std::wstring const kInstructionPattern{
    L"48 8D 05 ?? ?? ?? ?? 48 8D ?? ?? 48 8B ??"};
#else
#error "[HadesMem] Unsupported architecture."
#endif

  if (process.GetId() != ::GetCurrentProcessId())
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error{} << ErrorString{
        "This feature is only supported on the local process."});
  }

  auto instruction_ptr = static_cast<InstructionPtrT*>(
    hadesmem::Find(process,
                   L"ntdll.dll",
                   kInstructionPattern,
                   hadesmem::PatternFlags::kThrowOnUnmatch,
                   0U));

  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "process=%d, instruction=%p", process.GetId(), instruction_ptr);

  winternl::PVECTORED_HANDLER_LIST vectored_handler_list = nullptr;

#if defined(HADESMEM_DETAIL_ARCH_X86)
  vectored_handler_list = reinterpret_cast<winternl::PVECTORED_HANDLER_LIST>(
    instruction_ptr->Operand);
#elif defined(HADESMEM_DETAIL_ARCH_X64)
  // Because of x64 RIP relative addressing.
  vectored_handler_list = reinterpret_cast<winternl::PVECTORED_HANDLER_LIST>(
    reinterpret_cast<std::uint8_t*>(instruction_ptr) +
    instruction_ptr->Operand + sizeof(InstructionPtrT));
#else
#error "[HadesMem] Unsupported architecture."
#endif

  return vectored_handler_list->First;
}
}
}
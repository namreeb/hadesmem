// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "disassemble.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <memory>
#include <vector>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <udis86.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/detail/str_conv.hpp>
#include <hadesmem/pelib/nt_headers.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>

#include "main.hpp"
#include "print.hpp"
#include "warning.hpp"

namespace
{

std::size_t GetBytesToEndOfFile(hadesmem::PeFile const& pe_file, void* address)
{
  return (reinterpret_cast<std::uintptr_t>(pe_file.GetBase()) +
          pe_file.GetSize()) -
         reinterpret_cast<std::uintptr_t>(address);
}

std::uintptr_t GetEip(hadesmem::Process const& process,
                      hadesmem::PeFile const& pe_file,
                      std::uintptr_t rva,
                      void* va)
{
  if (pe_file.GetType() == hadesmem::PeFileType::Data)
  {
    hadesmem::NtHeaders const nt_headers(process, pe_file);
    return nt_headers.GetImageBase() + rva;
  }
  else
  {
    return reinterpret_cast<std::uintptr_t>(va);
  }
}
}

void DisassembleEp(hadesmem::Process const& process,
                   hadesmem::PeFile const& pe_file,
                   std::uintptr_t ep_rva,
                   void* ep_va,
                   std::size_t tabs)
{
  if (!ep_va)
  {
    return;
  }

  std::wostream& out = std::wcout;

  ud_t ud_obj;
  ud_init(&ud_obj);
  // Get the number of bytes from the EP to the end of the file.
  std::size_t max_buffer_size = GetBytesToEndOfFile(pe_file, ep_va);
  // Clamp the amount of data read to the theoretical maximum.
  std::size_t const kMaxInstructions = 10U;
  std::size_t const kMaxInstructionLen = 15U;
  std::size_t const kMaxInstructionsBytes =
    kMaxInstructions * kMaxInstructionLen;
  max_buffer_size = (std::min)(max_buffer_size, kMaxInstructionsBytes);
  auto const disasm_buf =
    hadesmem::ReadVector<std::uint8_t>(process, ep_va, max_buffer_size);
  ud_set_input_buffer(&ud_obj, disasm_buf.data(), max_buffer_size);
  ud_set_syntax(&ud_obj, UD_SYN_INTEL);
  ud_set_pc(&ud_obj, GetEip(process, pe_file, ep_rva, ep_va));
#if defined(_M_AMD64)
  ud_set_mode(&ud_obj, 64);
#elif defined(_M_IX86)
  ud_set_mode(&ud_obj, 32);
#else
#error "[HadesMem] Unsupported architecture."
#endif

  // Be pessimistic. Use the minimum theoretical amount of instrutions we could
  // fit in our buffer.
  std::size_t const instruction_count = max_buffer_size / kMaxInstructionLen;
  for (std::size_t i = 0U; i < instruction_count; ++i)
  {
    std::uint32_t const len = ud_disassemble(&ud_obj);
    if (len == 0)
    {
      WriteNormal(out, L"WARNING! Disassembly failed.", tabs);
      // If we can't disassemble at least 5 instructions there's probably
      // something strange about the function. Even in the case of a nullsub
      // there is typically some INT3 or NOP padding after it...
      WarnForCurrentFile(i < 5U ? WarningType::kUnsupported
                                : WarningType::kSuspicious);
      break;
    }

    char const* const asm_str = ud_insn_asm(&ud_obj);
    HADESMEM_DETAIL_ASSERT(asm_str);
    char const* const asm_bytes_str = ud_insn_hex(&ud_obj);
    HADESMEM_DETAIL_ASSERT(asm_bytes_str);
    auto const diasm_line =
      hadesmem::detail::MultiByteToWideChar(asm_str) + L" (" +
      hadesmem::detail::MultiByteToWideChar(asm_bytes_str) + L")";
    WriteNormal(out, diasm_line, tabs);
  }
}

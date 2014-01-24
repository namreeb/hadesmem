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
  // TODO: Fix this so we don't risk overflow etc.
  std::size_t size = 0U;
  if (pe_file.GetType() == hadesmem::PeFileType::Data)
  {
    // TODO: Don't read so much unnecessary data. We know the maximum
    // instruction length for the architecture, so we should at least clamp it
    // based on that (and the max number of instructions to disassemble). This
    // could also fail for 'hostile' PE files.
    size = (reinterpret_cast<std::uintptr_t>(pe_file.GetBase()) +
            pe_file.GetSize()) -
           reinterpret_cast<std::uintptr_t>(ep_va);
  }
  else
  {
    // TODO: Fix this.
    auto const mbi = hadesmem::detail::Query(process, ep_va);
    size = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress) + mbi.RegionSize -
           reinterpret_cast<std::uintptr_t>(ep_va);
  }
  auto const disasm_buf =
    hadesmem::ReadVector<std::uint8_t>(process, ep_va, size);
  ud_set_input_buffer(&ud_obj, disasm_buf.data(), size);
  ud_set_syntax(&ud_obj, UD_SYN_INTEL);
  if (pe_file.GetType() == hadesmem::PeFileType::Data)
  {
    hadesmem::NtHeaders const nt_headers(process, pe_file);
    auto const eip = ep_rva + nt_headers.GetImageBase();
    ud_set_pc(&ud_obj, eip);
  }
  else
  {
    ud_set_pc(&ud_obj, reinterpret_cast<std::uintptr_t>(ep_va));
  }
#if defined(_M_AMD64)
  ud_set_mode(&ud_obj, 64);
#elif defined(_M_IX86)
  ud_set_mode(&ud_obj, 32);
#else
#error "[HadesMem] Unsupported architecture."
#endif

  // TODO: Experiment to find the "right" number of instructions to try and
  // disassemble.
  for (std::size_t i = 0; i < 10; ++i)
  {
    std::uint32_t const len = ud_disassemble(&ud_obj);
    if (len == 0)
    {
      WriteNormal(out, L"WARNING! Disassembly failed.", tabs);
      WarnForCurrentFile(WarningType::kUnsupported);
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

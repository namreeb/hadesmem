// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "tone_mapping.hpp"

#include <iostream>

#include <hadesmem/error.hpp>
#include <hadesmem/find_pattern.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

void SetToneMappingType(hadesmem::Process const& process, std::uint32_t type)
{
  std::cout << "\nPreparing to set tone mapping type.\n";

  // eso.live.1.1.2.995904 (dumped with module base of 0x00960000)
  // .text:00C74BCB                 mov     eax, ds:dword_18BC774
  // .text:00C74BD0                 cmp     eax, ebx
  // .text:00C74BD2                 jz      short loc_C74BE0
  // .text:00C74BD4                 push    eax
  // .text:00C74BD5                 lea     ecx, [ebp+var_88]
  auto const fader_flag_ref = static_cast<std::uint8_t*>(
    hadesmem::Find(process,
                   L"",
                   L"A1 ?? ?? ?? ?? 3B C3 74 0C 50",
                   hadesmem::PatternFlags::kThrowOnUnmatch,
                   0));
  std::cout << "Got tone mapping type ref. ["
            << static_cast<void*>(fader_flag_ref) << "].\n";

  auto const kFaderFlagRefOffset = 0x01;
  auto const fader_flag_ptr = hadesmem::Read<std::uint8_t*>(
    process, fader_flag_ref + kFaderFlagRefOffset);
  std::cout << "Got tone mapping type ptr. ["
            << static_cast<void*>(fader_flag_ptr) << "].\n";

  auto const old_type = hadesmem::Read<std::uint8_t>(process, fader_flag_ptr);
  std::cout << "Old tone mapping type is " << old_type << ".\n";
  hadesmem::Write(process, fader_flag_ptr, type);
  std::cout << "New tone mapping type is " << type << ".\n";
}

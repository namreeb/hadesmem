// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "3d.hpp"

#include <iostream>

#include <hadesmem/error.hpp>
#include <hadesmem/find_pattern.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

void Toggle3D(hadesmem::Process const& process)
{
  std::cout << "\nPreparing to toggle 3D.\n";

  // eso.live.1.1.2.995904 (dumped with module base of 0x00960000)
  // .text:00C7A510                 cmp     ds:byte_18BC755, 0
  // .text:00C7A517                 mov     eax, [esi+8]
  // .text:00C7A51A                 jz      short loc_C7A52C
  // .text:00C7A51C                 fst     dword ptr [eax+0F0h]
  // .text:00C7A522                 mov     dword ptr [eax+0ECh], 5
  auto const anaglyph_flag_ref = static_cast<std::uint8_t*>(
    hadesmem::Find(process,
                   L"",
                   L"D9 90 F0 00 00 00 C7 80 EC 00 00 00 05 00 00 00",
                   hadesmem::PatternFlags::kThrowOnUnmatch,
                   0));
  std::cout << "Got 3D flag ref. [" << static_cast<void*>(anaglyph_flag_ref)
            << "].\n";

  auto const kAnaglyphFlagRefOffset = 0x0A;
  auto const anaglyph_flag_ptr = hadesmem::Read<std::uint8_t*>(
    process, anaglyph_flag_ref - kAnaglyphFlagRefOffset);
  std::cout << "Got 3D flag ptr. [" << static_cast<void*>(anaglyph_flag_ptr)
            << "].\n";

  auto const anaglyph_flag =
    hadesmem::Read<std::uint8_t>(process, anaglyph_flag_ptr);
  std::cout << "Old 3D flag is " << static_cast<std::uint32_t>(anaglyph_flag)
            << ".\n";
  auto const new_anaglyph_flag = static_cast<std::uint8_t>(!anaglyph_flag);
  hadesmem::Write(process, anaglyph_flag_ptr, new_anaglyph_flag);
  std::cout << "New 3D flag is "
            << static_cast<std::uint32_t>(new_anaglyph_flag) << ".\n";
}

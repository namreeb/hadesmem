// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "fog.hpp"

#include <iostream>

#include <hadesmem/error.hpp>
#include <hadesmem/find_pattern.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

void ToggleFog(hadesmem::Process const& process)
{
  std::cout << "\nPreparing to toggle fog.\n";

  // eso.live.1.5.7.1091867 (dumped with module base of 0x00F70000)
  // .text:019122CF                 lea     ecx, [ebp-0C0h]
  // .text:019122D5                 call    sub_1919430
  // .text:019122DA                 cmp     ds:byte_21282AB, bl
  auto const fog_flag_ref = static_cast<std::uint8_t*>(
    hadesmem::Find(process,
                   L"",
                   L"8D 8D 40 FF FF FF E8 ?? ?? ?? ?? 38 1D",
                   hadesmem::PatternFlags::kThrowOnUnmatch,
                   0));
  std::cout << "Got fog flag ref. [" << static_cast<void*>(fog_flag_ref)
            << "].\n";

  auto const kFogFlagRefOffset = 0x0D;
  auto const fog_flag_ptr =
    hadesmem::Read<std::uint8_t*>(process, fog_flag_ref + kFogFlagRefOffset);
  std::cout << "Got fog flag ptr. [" << static_cast<void*>(fog_flag_ptr)
            << "].\n";

  auto const fog_flag = hadesmem::Read<std::uint8_t>(process, fog_flag_ptr);
  std::cout << "Old fog flag is " << static_cast<std::uint32_t>(fog_flag)
            << ".\n";
  auto const new_fog_flag = static_cast<std::uint8_t>(!fog_flag);
  hadesmem::Write(process, fog_flag_ptr, new_fog_flag);
  std::cout << "New fog flag is " << static_cast<std::uint32_t>(new_fog_flag)
            << ".\n";
}

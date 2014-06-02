// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include <iostream>

#include <hadesmem/error.hpp>
#include <hadesmem/find_pattern.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

void ToggleFog(hadesmem::Process const& process)
{
  std::cout << "\nPreparing to toggle fog.\n";

  // eso.live.1.1.2.995904 (dumped with module base of 0x00960000)
  // .text:0107623D                 mov     byte ptr [ebp+var_4], 1
  // .text:01076241                 call    sub_107B390
  // .text:01076246                 cmp     ds:byte_18BC757, bl
  auto const fog_flag_ref = static_cast<std::uint8_t*>(
    hadesmem::Find(process,
                   L"",
                   L"C6 45 FC 01 E8 ?? ?? ?? ?? 38 1D",
                   hadesmem::PatternFlags::kThrowOnUnmatch,
                   0));
  std::cout << "Got fog flag ref. [" << static_cast<void*>(fog_flag_ref)
            << "].\n";

  auto const kFogFlagRefOffset = 0x0B;
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

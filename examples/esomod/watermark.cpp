// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include <iostream>

#include <hadesmem/error.hpp>
#include <hadesmem/find_pattern.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

void ToggleWatermark(hadesmem::Process const& process)
{
  std::cout << "\nPreparing to toggle watermark.\n";

  // eso.live.1.1.2.995904 (dumped with module base of 0x00960000)
  // .text:00C79D73                 cmp     byte ptr [esi+0Dh], 0
  // .text:00C79D77                 jz      short loc_C79DA8
  // .text:00C79D79                 cmp     ds:byte_18BC765, 0
  auto const watermark_flag_ref = static_cast<std::uint8_t*>(
    hadesmem::Find(process,
                   L"",
                   L"80 7E 0D 00 74 2F 80 3D",
                   hadesmem::PatternFlags::kThrowOnUnmatch,
                   0));
  std::cout << "Got watermark flag ref. ["
            << static_cast<void*>(watermark_flag_ref) << "].\n";

  auto const kWatermarkFlagRefOffset = 0x08;
  auto const watermark_flag_ptr = hadesmem::Read<std::uint8_t*>(
    process, watermark_flag_ref + kWatermarkFlagRefOffset);
  std::cout << "Got watermark flag ptr. ["
            << static_cast<void*>(watermark_flag_ptr) << "].\n";

  auto const watermark_flag =
    hadesmem::Read<std::uint8_t>(process, watermark_flag_ptr);
  std::cout << "Old watermark flag is "
            << static_cast<std::uint32_t>(watermark_flag) << ".\n";
  auto const new_watermark_flag = static_cast<std::uint8_t>(!watermark_flag);
  hadesmem::Write(process, watermark_flag_ptr, new_watermark_flag);
  std::cout << "New watermark flag is "
            << static_cast<std::uint32_t>(new_watermark_flag) << ".\n";
}

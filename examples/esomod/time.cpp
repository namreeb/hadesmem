// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "tone_mapping.hpp"

#include <iostream>

#include <hadesmem/error.hpp>
#include <hadesmem/find_pattern.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

void SetTime(hadesmem::Process const& process, float time)
{
  std::cout << "\nPreparing to set time.\n";

  // eso.live.1.1.3.998958 (dumped with module base of 0x002D0000)
  // .text:006A7E56                 fiadd   [ebp+var_8]
  // .text:006A7E59                 fstp    flt_133BDB0
  auto const time_ref = static_cast<std::uint8_t*>(
    hadesmem::Find(process,
                   L"",
                   L"DA 45 F8 D9 1D",
                   hadesmem::PatternFlags::kThrowOnUnmatch,
                   0));
  std::cout << "Got time ref. [" << static_cast<void*>(time_ref) << "].\n";

  auto const kTimeRefOffset = 0x05;
  auto const time_ptr =
    hadesmem::Read<std::uint8_t*>(process, time_ref + kTimeRefOffset);
  std::cout << "Got time ptr. [" << static_cast<void*>(time_ptr) << "].\n";

  auto const old_time = hadesmem::Read<float>(process, time_ptr);
  std::cout << "Old time is " << old_time << ".\n";
  hadesmem::Write(process, time_ptr, time);
  std::cout << "New time is " << time << ".\n";
}

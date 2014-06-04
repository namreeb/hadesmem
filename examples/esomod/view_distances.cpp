// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "view_distances.hpp"

#include <iostream>

#include <hadesmem/error.hpp>
#include <hadesmem/find_pattern.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

void SetViewDistances(hadesmem::Process const& process,
                      float* min,
                      float* max,
                      float* value)
{
  std::cout << "\nPreparing to set view distances.\n";

  // eso.live.1.1.2.995904 (dumped with module base of 0x00960000)
  // .text:00AB4D6A                 shr     edx, 2
  // .text:00AB4D6D                 test    dl, 1
  // .text:00AB4D70                 jz      short loc_AB4DAD
  // .text:00AB4D72                 fld     flt_1B22D68
  // .text:00AB4D78                 fld     ds:flt_18BC738
  auto const max_and_cur_view_distance_ref = static_cast<std::uint8_t*>(
    hadesmem::Find(process,
                   L"",
                   L"C1 EA 02 F6 C2 01 74 ?? D9 05",
                   hadesmem::PatternFlags::kThrowOnUnmatch,
                   0));
  std::cout << "Got tone mapping type ref. ["
            << static_cast<void*>(max_and_cur_view_distance_ref) << "].\n";

  auto const kCurrentViewDistanceRefOffset = 0x0A;
  auto const cur_view_distance_ptr = hadesmem::Read<std::uint8_t*>(
    process, max_and_cur_view_distance_ref + kCurrentViewDistanceRefOffset);
  std::cout << "Got current view distance ptr. ["
            << static_cast<void*>(cur_view_distance_ptr) << "].\n";

  auto const kMaxViewDistanceRefOffset = 0x10;
  auto const max_view_distance_ptr = hadesmem::Read<std::uint8_t*>(
    process, max_and_cur_view_distance_ref + kMaxViewDistanceRefOffset);
  std::cout << "Got max view distance ptr. ["
            << static_cast<void*>(max_view_distance_ptr) << "].\n";

  auto const min_view_distance_ptr = max_view_distance_ptr - 0x04;
  std::cout << "Got min view distance ptr. ["
            << static_cast<void*>(min_view_distance_ptr) << "].\n";

  if (min)
  {
    auto const old_distance =
      hadesmem::Read<float>(process, min_view_distance_ptr);
    std::cout << "Old min view distance " << old_distance << ".\n";
    hadesmem::Write(process, min_view_distance_ptr, *min);
    std::cout << "New min view distance " << *min << ".\n";
  }

  if (max)
  {
    auto const old_distance =
      hadesmem::Read<float>(process, max_view_distance_ptr);
    std::cout << "Old max view distance " << old_distance << ".\n";
    hadesmem::Write(process, max_view_distance_ptr, *max);
    std::cout << "New max view distance " << *max << ".\n";
  }

  if (value)
  {
    auto const old_distance =
      hadesmem::Read<float>(process, cur_view_distance_ptr);
    std::cout << "Old current view distance " << old_distance << ".\n";
    hadesmem::Write(process, cur_view_distance_ptr, *value);
    std::cout << "New current view distance " << *value << ".\n";
  }
}

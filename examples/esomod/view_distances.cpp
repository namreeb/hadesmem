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

  // eso.rc.1.2.0.999025 (dumped with module base of 0x00A90000)
  // .text:00CBCB1A                 jz      short loc_CBCB5A
  // .text:00CBCB1C                 fld     flt_1E82620
  // .text:00CBCB22                 fld     ds:flt_1C0F76C
  // .text:00CBCB28                 fcom    st(1)
  auto const max_and_cur_view_distance_ref = static_cast<std::uint8_t*>(
    hadesmem::Find(process,
                   L"",
                   L"74 ?? D9 05 ?? ?? ?? ?? D9 05 ?? ?? ?? ?? D8 D1",
                   hadesmem::PatternFlags::kThrowOnUnmatch,
                   0));
  std::cout << "Got tone mapping type ref. ["
            << static_cast<void*>(max_and_cur_view_distance_ref) << "].\n";

  auto const kCurrentViewDistanceRefOffset = 0x04;
  auto const cur_view_distance_ptr = hadesmem::Read<std::uint8_t*>(
    process, max_and_cur_view_distance_ref + kCurrentViewDistanceRefOffset);
  std::cout << "Got current view distance ptr. ["
            << static_cast<void*>(cur_view_distance_ptr) << "].\n";

  auto const kMaxViewDistanceRefOffset = 0x0A;
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

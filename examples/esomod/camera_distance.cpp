// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "camera_distance.hpp"

#include <iostream>

#include <hadesmem/error.hpp>
#include <hadesmem/find_pattern.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

void SetMaxCameraDistance(hadesmem::Process const& process, float value)
{
  std::cout << "\nPreparing to set max camera distance.\n";

  // eso.live.1.1.3.998958 (dumped with module base of 002D0000)
  // .text:0038DFF1                   fld1
  // .text:0038DFF3                   mov     ecx, dword_153C96C
  // .text:0038DFF9                   fstp    [ebp+var_4]
  // .text:0038DFFC                   call    sub_43C680
  auto const global_pointer_manager_ref = static_cast<std::uint8_t*>(
    hadesmem::Find(process,
                   L"",
                   L"D9 E8 8B 0D ?? ?? ?? ?? D9 5D FC E8",
                   hadesmem::PatternFlags::kThrowOnUnmatch,
                   0));
  std::cout << "Got global pointer manager ref. ["
            << static_cast<void*>(global_pointer_manager_ref) << "].\n";

  auto const kGlobalPointerManagerRefOffset = 0x04;
  auto const global_pointer_manager_ptr = hadesmem::Read<std::uint8_t*>(
    process, global_pointer_manager_ref + kGlobalPointerManagerRefOffset);
  std::cout << "Got global pointer manager ptr. ["
            << static_cast<void*>(global_pointer_manager_ptr) << "].\n";

  auto const global_pointer_manager =
    hadesmem::Read<std::uint8_t*>(process, global_pointer_manager_ptr);
  std::cout << "Got camera manager. ["
            << static_cast<void*>(global_pointer_manager) << "].\n";

  // eso.live.1.1.3.998958 (dumped with module base of 002D0000)
  // .text:0043C6D0                 mov     eax, [ecx + 4Ch]
  auto const kCameraConstraintsOffset = 0x4C;
  auto const camera_constraints = hadesmem::Read<std::uint8_t*>(
    process, global_pointer_manager + kCameraConstraintsOffset);
  std::cout << "Got camera constraints. ["
            << static_cast<void*>(camera_constraints) << "].\n";

  // eso.live.1.1.3.998958 (dumped with module base of 002D0000)
  // .text:0033AD65                 fld     dword ptr[esi + 64h]
  auto const kMaxCameraDistanceOffset = 0x64;
  auto const max_camera_distance =
    camera_constraints + kMaxCameraDistanceOffset;
  std::cout << "Writing max camera distance. ["
            << static_cast<void*>(max_camera_distance) << "].\n";
  hadesmem::Write(process, max_camera_distance, value);

  std::cout << "New max camera distance is " << value << ".\n";
}

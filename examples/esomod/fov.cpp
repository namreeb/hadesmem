// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "fov.hpp"

#include <iostream>

#include <hadesmem/error.hpp>
#include <hadesmem/find_pattern.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

namespace
{

float DegToRad(float f)
{
  auto const kPi = 3.14159265f;
  auto const kDegToRad = kPi / 180.0f;
  return f * kDegToRad;
}
}

void SetFov(hadesmem::Process const& process, float* third, float* first)
{
  std::cout << "\nPreparing to set FoV.\n";

  // eso.live.1.1.2.995904 (dumped with module base of 0x00960000)
  // .text:00A58435                 jnz     loc_A58641
  // .text:00A5843B                 mov     edx, dword_1BCA930
  auto const camera_manager_ref = static_cast<std::uint8_t*>(
    hadesmem::Find(process,
                   L"",
                   L"0F 85 ?? ?? ?? ?? 8B 15 ?? ?? ?? ?? 8B 4A 14",
                   hadesmem::PatternFlags::kThrowOnUnmatch,
                   0));
  std::cout << "Got camera manager ref. ["
            << static_cast<void*>(camera_manager_ref) << "].\n";

  auto const kCameraManagerRefOffset = 0x08;
  auto const camera_manager_ptr = hadesmem::Read<std::uint8_t*>(
    process, camera_manager_ref + kCameraManagerRefOffset);
  std::cout << "Got camera manager ptr. ["
            << static_cast<void*>(camera_manager_ptr) << "].\n";

  auto const camera_manager =
    hadesmem::Read<std::uint8_t*>(process, camera_manager_ptr);
  std::cout << "Got camera manager. [" << static_cast<void*>(camera_manager)
            << "].\n";

  // eso.live.1.1.2.995904 (dumped with module base of 0x00960000)
  // .text:00A72AB0                 cmp     dword ptr [ecx+14h], 0
  auto const kCameraOffset = 0x14;
  auto const camera =
    hadesmem::Read<std::uint8_t*>(process, camera_manager + kCameraOffset);
  std::cout << "Got camera. [" << static_cast<void*>(camera) << "].\n";

  if (third)
  {
    // eso.live.1.1.2.995904 (dumped with module base of 0x00960000)
    // .text:00A780C6                 fadd    dword ptr [esi+478h]
    // eso.live.1.1.2.995904 (dumped with module base of 0x00960000)
    // .text:00A79620                 fadd    dword ptr [esi+478h]
    auto const kCameraVertFov3pOffset = 0x478;
    auto const vert_fov_3p = camera + kCameraVertFov3pOffset;
    std::cout << "Writing 3rd person FoV. [" << static_cast<void*>(vert_fov_3p)
              << "].\n";
    hadesmem::Write(process, vert_fov_3p, DegToRad(*third));

    std::cout << "New 3rd person is " << *third << ".\n";
  }

  if (first)
  {
    // eso.live.1.1.2.995904 (dumped with module base of 0x00960000)
    // .text:00A780B5                 fld     dword ptr [esi+47Ch]
    // eso.live.1.1.2.995904 (dumped with module base of 0x00960000)
    // .text:00A794D4                 fld     dword ptr [esi+47Ch]
    auto const kCameraVertFov1pOffset = 0x47C;
    auto const vert_fov_1p = camera + kCameraVertFov1pOffset;
    std::cout << "Writing 1st person FoV. [" << static_cast<void*>(vert_fov_1p)
              << "].\n";
    hadesmem::Write(process, vert_fov_1p, DegToRad(*first));

    std::cout << "New 1st person is " << *first << ".\n";
  }
}

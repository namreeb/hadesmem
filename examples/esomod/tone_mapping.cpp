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
  auto tone_mapping_type_ref =
    static_cast<std::uint8_t*>(hadesmem::Find(process,
                                              L"",
                                              L"A1 ?? ?? ?? ?? 3B C3 74 0C 50",
                                              hadesmem::PatternFlags::kNone,
                                              0));
  auto tone_mapping_type_ref_offset = 0x01;
  if (!tone_mapping_type_ref)
  {
    // In 1.2.0 the code changed due to a new flag being added which toggles
    // tone mapping on or off, controlled by a new setting (aptly named
    // "TONE_MAPPING"). I couldn't see anywhere where the actual tone mapping
    // type was being set though so it seems a bit pointless, but maybe I've
    // simply overlooked it or they plan on adding that in the future... Either
    // way, I didn't look very hard and this new pattern works so I'll continue
    // doing it this way until the functionality is properly exposed in the UI.
    // eso.rc.1.2.0.999025 (dumped with module base of 0x00A90000)
    // .text:00E8A424                 jz      short loc_E8A443
    // .text:00E8A426                 lea     ecx, [ebp+var_C8]
    // .text:00E8A42C                 call    sub_1338A70
    // .text:00E8A431                 mov     ecx, ds:dword_1C0F7A8
    tone_mapping_type_ref = static_cast<std::uint8_t*>(
      hadesmem::Find(process,
                     L"",
                     L"74 1D 8D 8D ?? ?? ?? ?? E8 ?? ?? ?? ?? 8B 0D",
                     hadesmem::PatternFlags::kThrowOnUnmatch,
                     0));
    tone_mapping_type_ref_offset = 0x0F;
  }
  std::cout << "Got tone mapping type ref. ["
            << static_cast<void*>(tone_mapping_type_ref) << "].\n";

  auto const tone_mapping_type_ptr = hadesmem::Read<std::uint8_t*>(
    process, tone_mapping_type_ref + tone_mapping_type_ref_offset);
  std::cout << "Got tone mapping type ptr. ["
            << static_cast<void*>(tone_mapping_type_ptr) << "].\n";

  auto const old_type =
    hadesmem::Read<std::uint32_t>(process, tone_mapping_type_ptr);
  std::cout << "Old tone mapping type is " << old_type << ".\n";
  hadesmem::Write(process, tone_mapping_type_ptr, type);
  std::cout << "New tone mapping type is " << type << ".\n";
}

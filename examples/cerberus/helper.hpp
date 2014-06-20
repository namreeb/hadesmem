// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <atomic>
#include <memory>
#include <string>

#include <hadesmem/patcher.hpp>

namespace hadesmem
{

namespace cerberus
{

void UndetourFunc(std::wstring const& name,
                  std::unique_ptr<hadesmem::PatchDetour>& detour,
                  std::atomic<std::uint32_t>& ref_count,
                  bool remove);
}
}

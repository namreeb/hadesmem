// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <hadesmem/process.hpp>

namespace hadesmem
{
namespace cerberus
{
hadesmem::Process& GetThisProcess();
}
}

extern "C" __declspec(dllexport) DWORD_PTR Load() noexcept;

extern "C" __declspec(dllexport) DWORD_PTR Free() noexcept;

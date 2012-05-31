// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <cstddef>

#include <windows.h>

namespace hadesmem
{

class Process;

bool CanRead(Process const& process, LPCVOID address);

bool CanWrite(Process const& process, LPCVOID address);

bool CanExecute(Process const& process, LPCVOID address);

bool IsGuard(Process const& process, LPCVOID address);

PVOID Alloc(Process const& process, SIZE_T size);

void Free(Process const& process, LPVOID address);

void FlushInstructionCache(Process const& process, LPCVOID address, SIZE_T size);

DWORD Protect(Process const& process, LPVOID address, DWORD protect);

namespace detail
{

MEMORY_BASIC_INFORMATION Query(Process const& process, LPCVOID address);

void Read(Process const& process, LPVOID address, LPVOID out, std::size_t out_size);

}

}

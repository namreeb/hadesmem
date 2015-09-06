// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <string>

#include <hadesmem/detail/thread_pool.hpp>

void DumpFile(std::wstring const& path);

void DumpDir(std::wstring const& path, hadesmem::detail::ThreadPool& pool);

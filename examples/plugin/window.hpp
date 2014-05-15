// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>

#include <hadesmem/detail/smart_handle.hpp>

int PASCAL WindowThread(HINSTANCE hinst, HINSTANCE, LPSTR, int nShowCmd);

HWND& GetWindowHandle();

hadesmem::detail::SmartHandle& GetThreadHandle();

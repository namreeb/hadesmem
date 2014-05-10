// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>

int PASCAL WindowThread(HINSTANCE hinst, HINSTANCE, LPSTR, int nShowCmd);

HWND& GetWindowHandle();

HANDLE& GetThreadHandle();

// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include <utility>

#include <windows.h>

#pragma once

void DetourD3D11(HMODULE base);

void DetourDXGI(HMODULE base);

void UndetourD3D11(bool remove);

void UndetourDXGI(bool remove);

std::pair<void*, SIZE_T>& GetD3D11Module();

std::pair<void*, SIZE_T>& GetDXGIModule();

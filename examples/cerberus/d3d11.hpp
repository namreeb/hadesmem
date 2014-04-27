// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <utility>

#include <windows.h>

#include <hadesmem/config.hpp>

void InitializeD3D11();

void DetourD3D11(HMODULE base);

void DetourDXGI(HMODULE base);

void UndetourD3D11(bool remove);

void UndetourDXGI(bool remove);

std::pair<void*, SIZE_T>& GetD3D11Module() HADESMEM_DETAIL_NOEXCEPT;

std::pair<void*, SIZE_T>& GetDXGIModule() HADESMEM_DETAIL_NOEXCEPT;

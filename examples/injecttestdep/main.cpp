// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include <windows.h>

#include <hadesmem/config.hpp>

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved);

extern "C" HADESMEM_DETAIL_DLLEXPORT DWORD_PTR InjectTestDep_Foo();

extern "C" HADESMEM_DETAIL_DLLEXPORT DWORD_PTR InjectTestDep_Foo()
{
  return 0;
}

BOOL WINAPI
  DllMain(HINSTANCE /*instance*/, DWORD /*reason*/, LPVOID /*reserved*/)
{
  return TRUE;
}

// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#include <windows.h>

#include <hadesmem/config.hpp>

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved);

// This is required because of a bug in Clang's dllexport support on Windows, 
// this is worked around by using a linker flag to export all symbols 
// unconditionally.
// TODO: Remove this hack once Clang has been fixed.
// TODO: Move this to config header.
#ifdef HADESMEM_CLANG
#define HADESMEM_DLLEXPORT  
#else
#define HADESMEM_DLLEXPORT __declspec(dllexport)
#endif

extern "C" HADESMEM_DLLEXPORT DWORD_PTR InjectTestDep_Foo();

extern "C" HADESMEM_DLLEXPORT DWORD_PTR InjectTestDep_Foo()
{
  return 0;
}

BOOL WINAPI DllMain(HINSTANCE /*instance*/, DWORD /*reason*/, 
  LPVOID /*reserved*/)
{
  return TRUE;
}

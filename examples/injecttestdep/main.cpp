// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE /*instance*/, DWORD /*reason*/, 
  LPVOID /*reserved*/);

extern "C" __declspec(dllexport) DWORD InjectTestDep_Foo();

extern "C" __declspec(dllexport) DWORD InjectTestDep_Foo()
{
  return 0;
}

BOOL WINAPI DllMain(HINSTANCE /*instance*/, DWORD /*reason*/, 
  LPVOID /*reserved*/)
{
  return TRUE;
}

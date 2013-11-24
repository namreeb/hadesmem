// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <windows.h>

#include <hadesmem/config.hpp>

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved);

extern "C" HADESMEM_DETAIL_DLLEXPORT DWORD_PTR Load();

extern "C" HADESMEM_DETAIL_DLLEXPORT DWORD_PTR Free();

extern "C" __declspec(dllimport) DWORD_PTR InjectTestDep_Foo();

extern bool g_alloced_console;

bool g_alloced_console = false;

extern "C" HADESMEM_DETAIL_DLLEXPORT DWORD_PTR Load()
{
    InjectTestDep_Foo();

    if (!AllocConsole())
    {
        return GetLastError();
    }

    g_alloced_console = true;

    return 0;
}

extern "C" HADESMEM_DETAIL_DLLEXPORT DWORD_PTR Free()
{
    if (g_alloced_console)
    {
        if (!FreeConsole())
        {
            return GetLastError();
        }
    }

    return 0;
}

BOOL WINAPI DllMain(HINSTANCE /*instance*/, DWORD /*reason*/,
    LPVOID /*reserved*/)
{
    return TRUE;
}

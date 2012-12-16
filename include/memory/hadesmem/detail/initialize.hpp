// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <locale>

#include <windows.h>

namespace hadesmem
{

namespace detail
{

// Disables the user-mode callback exception filter present in 64-bit versions 
// of Windows. Microsoft Support article KB976038 (http://goo.gl/tlIsu).
void DisableUserModeCallbackExceptionFilter();

// Enables CRT debug flags for memory leak detection in debug builds.
void EnableCrtDebugFlags();

// Forces termination of the application if heap corruption is detected.
// Recommended as per Microsoft article "Windows ISV Software Security 
// Defenses" (http://goo.gl/d7WHe).
void EnableTerminationOnHeapCorruption();

// Custom 'bottom up randomization' implementation similar to that of EMET.
// Modified version of code by Didier Stevens (http://goo.gl/LzOAA).
void EnableBottomUpRand();

// Generates a new UTF-8 based locale object, sets the global locale, and 
// imbues all known static streams.
std::locale ImbueAllDefault();

// Sets the global locale, and imbues all existing static streams with the 
// new locale (including 3rd party libraries like Boost.Filesystem).
std::locale ImbueAll(std::locale const& locale);

}

}

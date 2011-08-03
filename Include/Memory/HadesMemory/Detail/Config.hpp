/*
This file is part of HadesMem.
Copyright (C) 2011 Joshua Boyce (a.k.a. RaptorFactor).
<http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

HadesMem is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

HadesMem is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with HadesMem.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

// Boost
#include <boost/config.hpp>

// Version numbers
#define HADES_VERSION_MAJOR 1
#define HADES_VERSION_MINOR 3
#define HADES_VERSION_PATCH 0

// Full version number
#define HADES_VERSION_FULL ((HADES_VERSION_MAJOR * 10000) + \
(HADES_VERSION_MINOR * 100) + HADES_VERSION_PATCH)

// Full version number string generator helper
#define HADES_VERSION_FULL_STRING_GEN_EXP(x, y, z) "v" #x "." #y "." #z

// Full version number string generator
#define HADES_VERSION_FULL_STRING_GEN(x, y, z) \
HADES_VERSION_FULL_STRING_GEN_EXP(x, y, z)

// Full version number string
#define HADES_VERSION_FULL_STRING HADES_VERSION_FULL_STRING_GEN(\
HADES_VERSION_MAJOR, HADES_VERSION_MINOR, HADES_VERSION_PATCH)

// Compiler detection
#if defined(BOOST_MSVC)
  #if _MSC_VER < 1600
    #error "[HadesMem] MSVC 10 or later required."
  #endif
  #define HADES_MSVC
#elif defined(BOOST_INTEL)
  #if __INTEL_COMPILER < 1200
    #error "[HadesMem] ICC 12 or later required."
  #endif
  #define HADES_INTEL
#elif defined(BOOST_CLANG)
  // Todo: Do feature checks here.
  #define HADES_CLANG
// Detect GCC
// Note: This check must be last as both ICC and Clang also define __GNUC__ 
#elif defined(__GNUC__)
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
  #if GCC_VERSION < 40600
    #error "[HadesMem] GCC 4.6 or later required."
  #endif
  #define HADES_GCC
#endif

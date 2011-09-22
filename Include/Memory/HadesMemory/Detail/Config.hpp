// Copyright Joshua Boyce 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

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

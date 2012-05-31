#pragma once

#include <boost/config.hpp>
#include <boost/version.hpp>

#if (BOOST_VERSION < 104900)
#error "[HadesMem] Boost 1.49.0 or later is required."
#endif // #if (BOOST_VERSION < 014900)

#if defined(HADESMEM_MSVC)
#if (_MSC_VER < 1700)
#error "[HadesMem] MSVC 11 or later is required."
#endif // #if (_MSC_VER < 1700)
#endif // #if defined(HADESMEM_MSVC)

#if defined(HADESMEM_INTEL)
#if (__INTEL_COMPILER < 1210)
#error "[HadesMem] ICC 12.1 or later is required."
#endif // #if (__INTEL_COMPILER < 1210)
#endif // #if defined(HADESMEM_INTEL)

#if defined(HADESMEM_GCC)
#if ((__GNUC__ < 4) || (__GNUC__ == 4 && __GNUC_MINOR__ < 7))
#error "[HadesMem] GCC 4.7.0 or later is required."
#endif // #if ((__GNUC__ < 4) || (__GNUC__ == 4 && __GNUC_MINOR__ < 7))
#endif // #if defined(HADESMEM_GCC)

#if defined(HADESMSM_CLANG)
#if ((__clang_major__ < 3) || (__clang_major__ == 3 && __clang_minor__ < 1))
#error "[HadesMem] Clang 3.1 or later is required."
#endif // #if ((__clang_major__ < 3) || (__clang_major__ == 3 && 
// __clang_minor__ < 1))
#endif // #if defined(HADESMSM_CLANG)

#define HADESMEM_VERSION_MAJOR 2
#define HADESMEM_VERSION_MINOR 0
#define HADESMEM_VERSION_PATCH 0

#define HADESMEM_VERSION_STRING_GEN_EXP(x, y, z) "v" #x "." #y "." #z

#define HADESMEM_VERSION_STRING_GEN(x, y, z) \
HADESMEM_VERSION_STRING_GEN_EXP(x, y, z)

#define HADESMEM_VERSION_STRING HADESMEM_VERSION_STRING_GEN(\
HADESMEM_VERSION_MAJOR, HADESMEM_VERSION_MINOR, HADESMEM_VERSION_PATCH)

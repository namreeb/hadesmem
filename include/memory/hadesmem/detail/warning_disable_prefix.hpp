// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/config.hpp>

#if defined(HADESMEM_MSVC)
#pragma warning(push, 1)
#pragma warning(disable : 4702 4996)
#pragma warning(disable : 6011 6239 6244 6246 6295 6326 6334 6385 6386 6387)
#pragma warning(disable : 28159 28197 28285)
#endif // #if defined(HADESMEM_MSVC)

#if defined(HADESMEM_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-pedantic"
#pragma GCC diagnostic ignored "-Wreorder"
#pragma GCC diagnostic ignored "-Wattributes"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wunused-value"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic ignored "-Wstrict-overflow"
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#endif // #if defined(HADESMEM_GCC)

#if defined(HADESMEM_INTEL)
#pragma warning(push, 1)
#pragma warning(disable : 9 10 66 82 177 367 411 504 693 780 809 869 1879 1885)
#pragma warning(disable : 3280 3346)
#endif // #if defined(HADESMEM_MSVC)

#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wgnu"
#pragma GCC diagnostic ignored "-Wundef"
#pragma GCC diagnostic ignored "-Wpadded"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wextra-semi"
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wswitch-enum"
#pragma GCC diagnostic ignored "-Wweak-vtables"
#pragma GCC diagnostic ignored "-Wdocumentation"
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wunreachable-code"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#pragma GCC diagnostic ignored "-Wshift-sign-overflow"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#pragma GCC diagnostic ignored "-Wcovered-switch-default"
#pragma GCC diagnostic ignored "-Wconditional-uninitialized"
#endif // #if defined(HADESMEM_CLANG)

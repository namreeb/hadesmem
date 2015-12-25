// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <memory>

#include <windows.h>

#include <hadesmem/detail/static_assert.hpp>

#define HADESMEM_VERSION_MAJOR 2
#define HADESMEM_VERSION_MINOR 0
#define HADESMEM_VERSION_PATCH 0

// TODO: Support different versioning modes once we 'hit' v2.0.0. For now just
// hardcode in 'Dev' mode.
#define HADESMEM_DETAIL_VERSION_STRING_GEN_EXP(x, y, z)                        \
  "v" #x "." #y "." #z "-Dev"

#define HADESMEM_DETAIL_VERSION_STRING_GEN(x, y, z)                            \
  HADESMEM_DETAIL_VERSION_STRING_GEN_EXP(x, y, z)

#define HADESMEM_VERSION_STRING                                                \
  HADESMEM_DETAIL_VERSION_STRING_GEN(                                          \
    HADESMEM_VERSION_MAJOR, HADESMEM_VERSION_MINOR, HADESMEM_VERSION_PATCH)

#if defined(_M_IX86)
#define HADESMEM_DETAIL_ARCH_X86
#elif defined(_M_AMD64)
#define HADESMEM_DETAIL_ARCH_X64
#else // #if defined(_M_IX86)
// #elif defined(_M_AMD64)
#error "[HadesMem] Unsupported architecture."
#endif // #if defined(_M_IX86)
// #elif defined(_M_AMD64)

#if !(defined(HADESMEM_DETAIL_ARCH_X64) ||                                     \
      (defined(HADESMEM_DETAIL_ARCH_X86) && _M_IX86_FP >= 2))
#define HADESMEM_DETAIL_NO_VECTORCALL
#endif // !(defined(HADESMEM_DETAIL_ARCH_X64) ||
       // (defined(HADESMEM_DETAIL_ARCH_X86) && _M_IX86_FP >= 2))

// Approximate equivalent of MAX_PATH for Unicode APIs.
// See: http://bit.ly/17CCZFX
#define HADESMEM_DETAIL_MAX_PATH_UNICODE (1 << 15)

// Every effort is made to NOT assume the below is true across the entire
// codebase, but for the Call module it is unavoidable. If adding support for
// another architecture, this may need adjusting. However, if anywhere other
// than here and Call needs adjusting, it is probably a bug and should be
// reported.
HADESMEM_DETAIL_STATIC_ASSERT(sizeof(DWORD) == 4);
HADESMEM_DETAIL_STATIC_ASSERT(sizeof(DWORD32) == 4);
HADESMEM_DETAIL_STATIC_ASSERT(sizeof(DWORD64) == 8);
HADESMEM_DETAIL_STATIC_ASSERT(sizeof(float) == 4);
HADESMEM_DETAIL_STATIC_ASSERT(sizeof(float) == sizeof(DWORD));
HADESMEM_DETAIL_STATIC_ASSERT(sizeof(double) == 8);
HADESMEM_DETAIL_STATIC_ASSERT(sizeof(double) == sizeof(DWORD64));

// While every effort is made to not rely on the below, it is unavoidable
// when manually implementing functions such as GetProcAddress, which is
// required by the Injector.
HADESMEM_DETAIL_STATIC_ASSERT(sizeof(FARPROC) == sizeof(void*));

// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <memory>

#include <windows.h>

#include <hadesmem/detail/static_assert.hpp>

// Allow the user to override the compiler detection.
#if !defined(HADESMEM_CLANG) && \
    !defined(HADESMEM_INTEL) && \
    !defined(HADESMEM_GCC) && \
    !defined(HADESMEM_MSVC)

#if defined(__clang__)
#define HADESMEM_CLANG
#elif defined(__INTEL_COMPILER)
#define HADESMEM_INTEL
#elif defined(__GNUC__)
#define HADESMEM_GCC
#elif defined(_MSC_VER)
#define HADESMEM_MSVC
#else
// I don't like having to do this, but given that some small parts of the 
// code base rely on 'questionable' interpretations of the standard it's 
// safer to verify before continuing.
#error "[HadesMem] Unsupported compiler."
#endif

#endif // #if !defined(HADESMEM_CLANG) && 
// !defined(HADESMEM_INTEL) && 
// !defined(HADESMEM_GCC) && 
// !defined(HADESMEM_MSVC)

#define HADESMEM_VERSION_MAJOR 2
#define HADESMEM_VERSION_MINOR 0
#define HADESMEM_VERSION_PATCH 0

#define HADESMEM_DETAIL_VERSION_STRING_GEN_EXP(x, y, z) "v" #x "." #y "." #z

#define HADESMEM_DETAIL_VERSION_STRING_GEN(x, y, z) \
    HADESMEM_DETAIL_VERSION_STRING_GEN_EXP(x, y, z)

#define HADESMEM_VERSION_STRING HADESMEM_DETAIL_VERSION_STRING_GEN(\
    HADESMEM_VERSION_MAJOR, HADESMEM_VERSION_MINOR, HADESMEM_VERSION_PATCH)

#if defined(HADESMEM_MSVC)
#define HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3
#endif // #if defined(HADESMEM_MSVC)

#if defined(HADESMEM_MSVC)
#define HADESMEM_DETAIL_NO_NOEXCEPT
#endif // #if defined(HADESMEM_MSVC)

#if defined(HADESMEM_MSVC)
#define HADESMEM_DETAIL_NO_CONSTEXPR
#endif // #if defined(HADESMEM_MSVC)

#if defined(HADESMEM_DETAIL_NO_NOEXCEPT)
#define HADESMEM_DETAIL_NOEXCEPT 
#define HADESMEM_DETAIL_NOEXCEPT_IF(Pred) 
#define HADESMEM_DETAIL_NOEXCEPT_EXPR(Expr) false
#else // #if defined(HADESMEM_DETAIL_NO_NOEXCEPT)
#define HADESMEM_DETAIL_NOEXCEPT noexcept
#define HADESMEM_DETAIL_NOEXCEPT_IF(Pred) noexcept((Pred))
#define HADESMEM_DETAIL_NOEXCEPT_EXPR(Expr) noexcept((Expr))
#endif // #if defined(HADESMEM_DETAIL_NO_NOEXCEPT)

#if defined(HADESMEM_DETAIL_NO_CONSTEXPR)
#define HADESMEM_DETAIL_CONSTEXPR
#else // #if defined(HADESMEM_DETAIL_NO_CONSTEXPR)
#define HADESMEM_DETAIL_CONSTEXPR constexpr
#endif // #if defined(HADESMEM_DETAIL_NO_CONSTEXPR)

#if defined(_M_IX86)
#define HADESMEM_DETAIL_ARCH_X86
#elif defined(_M_AMD64)
#define HADESMEM_DETAIL_ARCH_X64
#else // #if defined(_M_IX86)
// #elif defined(_M_AMD64)
#error "[HadesMem] Unsupported architecture."
#endif // #if defined(_M_IX86)
// #elif defined(_M_AMD64)

// This is required because of a bug in Clang's dllexport support on Windows, 
// this is worked around by using a linker flag to export all symbols 
// unconditionally.
// TODO: Remove this hack once Clang has been fixed.
#if defined(HADESMEM_CLANG)
#define HADESMEM_DETAIL_DLLEXPORT  
#else // #if defined(HADESMEM_CLANG)
#define HADESMEM_DETAIL_DLLEXPORT __declspec(dllexport)
#endif // #if defined(HADESMEM_CLANG)

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

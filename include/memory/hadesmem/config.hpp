// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <memory>

#include <windows.h>

#include <hadesmem/detail/static_assert.hpp>

#if defined(__clang__)
#define HADESMEM_CLANG
#elif defined(__INTEL_COMPILER)
#define HADESMEM_INTEL
#elif defined(__GNUC__)
#define HADESMEM_GCC
#elif defined(_MSC_VER)
#define HADESMEM_MSVC
#else
#error "[HadesMem] Unsupported compiler."
#endif

#define HADESMEM_VERSION_MAJOR 2
#define HADESMEM_VERSION_MINOR 0
#define HADESMEM_VERSION_PATCH 0

#define HADESMEM_VERSION_STRING_GEN_EXP(x, y, z) "v" #x "." #y "." #z

#define HADESMEM_VERSION_STRING_GEN(x, y, z) \
HADESMEM_VERSION_STRING_GEN_EXP(x, y, z)

#define HADESMEM_VERSION_STRING HADESMEM_VERSION_STRING_GEN(\
HADESMEM_VERSION_MAJOR, HADESMEM_VERSION_MINOR, HADESMEM_VERSION_PATCH)

#if defined(HADESMEM_MSVC)
#define HADESMEM_NO_DELETED_FUNCTIONS
#endif // #if defined(HADESMEM_MSVC)

#if defined(HADESMEM_MSVC)
#define HADESMEM_NO_NOEXCEPT
#endif // #if defined(HADESMEM_MSVC)

#if defined(HADESMEM_MSVC)
#define HADESMEM_NO_CONSTEXPR
#endif // #if defined(HADESMEM_MSVC)

#if defined(HADESMEM_MSVC)
#define HADESMEM_NO_VARIADIC_TEMPLATES
#endif // #if defined(HADESMEM_MSVC)

#if defined(HADESMEM_NO_DELETED_FUNCTIONS)
#define HADESMEM_DELETED_FUNCTION 
#else // #if defined(HADESMEM_NO_DELETED_FUNCTIONS)
#define HADESMEM_DELETED_FUNCTION = delete
#endif // #if defined(HADESMEM_NO_DELETED_FUNCTIONS)

#if defined(HADESMEM_NO_NOEXCEPT)
#define HADESMEM_NOEXCEPT 
#define HADESMEM_NOEXCEPT_IF(Pred) 
#define HADESMEM_NOEXCEPT_EXPR(Expr) false
#else // #if defined(HADESMEM_NO_NOEXCEPT)
#define HADESMEM_NOEXCEPT noexcept
#define HADESMEM_NOEXCEPT_IF(Pred) noexcept((Pred))
#define HADESMEM_NOEXCEPT_EXPR(Expr) noexcept((Expr))
#endif // #if defined(HADESMEM_NO_NOEXCEPT)

#if defined(HADESMEM_NO_CONSTEXPR)
#define HADESMEM_CONSTEXPR
#else // #if defined(HADESMEM_NO_CONSTEXPR)
#define HADESMEM_CONSTEXPR constexpr
#endif // #if defined(HADESMEM_NO_CONSTEXPR)

#if defined(_M_IX86)
#define HADESMEM_ARCH_X86
#endif // #if defined(_M_IX86)

#if defined(_M_AMD64)
#define HADESMEM_ARCH_X64
#endif // #if defined(_M_AMD64)

#if !defined(HADESMEM_ARCH_X86) && !defined(HADESMEM_ARCH_X64)
#error "[HadesMem] Unsupported architecture."
#endif // #if !defined(HADESMEM_ARCH_X86) && !defined(HADESMEM_ARCH_X64)

#if !defined(HADESMEM_CALL_MAX_ARGS)
#define HADESMEM_CALL_MAX_ARGS 10
#endif // #ifndef HADESMEM_CALL_MAX_ARGS

// This is required because of a bug in Clang's dllexport support on Windows, 
// this is worked around by using a linker flag to export all symbols 
// unconditionally.
// TODO: Remove this hack once Clang has been fixed.
#if defined(HADESMEM_CLANG)
#define HADESMEM_DLLEXPORT  
#else // #if defined(HADESMEM_CLANG)
#define HADESMEM_DLLEXPORT __declspec(dllexport)
#endif // #if defined(HADESMEM_CLANG)

// Approximate equivalent of MAX_PATH for Unicode APIs.
// See: http://goo.gl/1VVA3
#define HADESMEM_MAX_PATH_UNICODE (1 << 15)

// Every effort is made to NOT assume the below is true across the entire 
// codebase, but for the Call module it is unavoidable. If adding support for 
// another architecture, this may need adjusting. However, if anywhere other 
// than here and Call needs adjusting, it is probably a bug and should be 
// reported.
HADESMEM_STATIC_ASSERT(sizeof(DWORD) == 4);
HADESMEM_STATIC_ASSERT(sizeof(DWORD32) == 4);
HADESMEM_STATIC_ASSERT(sizeof(DWORD64) == 8);
HADESMEM_STATIC_ASSERT(sizeof(float) == 4);
HADESMEM_STATIC_ASSERT(sizeof(float) == sizeof(DWORD));
HADESMEM_STATIC_ASSERT(sizeof(double) == 8);
HADESMEM_STATIC_ASSERT(sizeof(double) == sizeof(DWORD64));

// While every effort is made to not rely on the below, it is unavoidable 
// when manually implementing functions such as GetProcAddress, which is 
// required by the Injector.
HADESMEM_STATIC_ASSERT(sizeof(FARPROC) == sizeof(void*));

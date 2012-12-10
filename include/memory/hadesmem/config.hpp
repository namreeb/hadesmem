// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

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

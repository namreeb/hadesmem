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
#endif

#if defined(HADESMEM_MSVC)
#define HADESMEM_NO_NOEXCEPT
#endif

#if defined(HADESMEM_MSVC)
#define HADESMEM_NO_VARIADIC_TEMPLATES
#endif

#if defined(HADESMEM_NO_DELETED_FUNCTIONS)
#define HADESMEM_DELETED_FUNCTION 
#else
#define HADESMEM_DELETED_FUNCTION = delete
#endif

#if defined(HADESMEM_NO_NOEXCEPT)
#define HADESMEM_NOEXCEPT 
#define HADESMEM_NOEXCEPT_IF(Pred) 
#define HADESMEM_NOEXCEPT_EXPR(Expr) false
#else
#define HADESMEM_NOEXCEPT noexcept
#define HADESMEM_NOEXCEPT_IF(Pred) noexcept((Pred))
#define HADESMEM_NOEXCEPT_EXPR(Expr) noexcept((Expr))
#endif

// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <hadesmem/config.hpp>
#include <hadesmem/detail/static_assert.hpp>

#if defined(HADESMEM_DETAIL_ARCH_X86)

#define HADESMEM_DETAIL_STATIC_ASSERT_X86(...)                                 \
  HADESMEM_DETAIL_STATIC_ASSERT(__VA_ARGS__)

#else

#define HADESMEM_DETAIL_STATIC_ASSERT_X86(...) static_assert(true, "")

#endif // #if defined(HADESMEM_DETAIL_ARCH_X86)

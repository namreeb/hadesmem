// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#define HADESMEM_DETAIL_STATIC_ASSERT(...)                                     \
  static_assert(__VA_ARGS__, #__VA_ARGS__)

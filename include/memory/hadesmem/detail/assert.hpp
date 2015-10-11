// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cassert>

// TODO: Actually use this everywhere! Especially for cases like overflow,
// underflow, preconditions, postconditions, etc.

#define HADESMEM_DETAIL_ASSERT(...) assert(__VA_ARGS__)

// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/assert.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#define HADESMEM_DETAIL_ASSERT(...) BOOST_ASSERT(__VA_ARGS__)

#define HADESMEM_VERIFY(...) BOOST_VERIFY(__VA_ARGS__)

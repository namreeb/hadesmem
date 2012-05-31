// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <boost/config.hpp>
#include <boost/version.hpp>

#define HADESMEM_VERSION_MAJOR 2
#define HADESMEM_VERSION_MINOR 0
#define HADESMEM_VERSION_PATCH 0

#define HADESMEM_VERSION_STRING_GEN_EXP(x, y, z) "v" #x "." #y "." #z

#define HADESMEM_VERSION_STRING_GEN(x, y, z) \
HADESMEM_VERSION_STRING_GEN_EXP(x, y, z)

#define HADESMEM_VERSION_STRING HADESMEM_VERSION_STRING_GEN(\
HADESMEM_VERSION_MAJOR, HADESMEM_VERSION_MINOR, HADESMEM_VERSION_PATCH)

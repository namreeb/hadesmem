/*
This file is part of HadesMem.
Copyright (C) 2011 Joshua Boyce (a.k.a. RaptorFactor).
<http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

HadesMem is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

HadesMem is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with HadesMem.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

// Boost
#include <boost/config.hpp>

#define HADES_VERSION_MAJOR 0
#define HADES_VERSION_MINOR 0
#define HADES_VERSION_PATCH 0

#define HADES_VERSION_FULL ((HADES_VERSION_MAJOR * 10000) + \
(HADES_VERSION_MINOR * 100) + HADES_VERSION_PATCH)

#define HADES_VERSION_FULL_STRING_GEN_EXP(x, y, z) #x "." #y "." #z

#define HADES_VERSION_FULL_STRING_GEN(x, y, z) \
HADES_VERSION_FULL_STRING_GEN_EXP(x, y, z)

#define HADES_VERSION_FULL_STRING HADES_VERSION_FULL_STRING_GEN(\
HADES_VERSION_MAJOR, HADES_VERSION_MINOR, HADES_VERSION_PATCH)

#if defined(BOOST_MSVC)
#define HADES_MSVC
#elif defined(BOOST_INTEL)
#define HADES_INTEL
#elif defined(BOOST_CLANG)
#define HADES_CLANG
// Detect GCC
// Note: This check must be last as both ICC and Clang also define __GNUC__ 
#elif defined(__GNUC__)
#define HADES_GCC
#endif

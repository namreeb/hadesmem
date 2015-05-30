// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/detail/smart_handle.hpp>

#include <hadesmem/detail/privilege.hpp>

namespace hadesmem
{
inline void GetSeDebugPrivilege()
{
  return detail::GetPrivilege(SE_DEBUG_NAME);
}
}

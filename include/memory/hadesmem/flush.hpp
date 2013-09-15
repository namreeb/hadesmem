// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>

#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>

namespace hadesmem
{

inline void FlushInstructionCache(Process const& process, LPCVOID address, 
  SIZE_T size)
{
  if (!::FlushInstructionCache(process.GetHandle(), address, size))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error() << 
      ErrorString("FlushInstructionCache failed.") << 
      ErrorCodeWinLast(last_error));
  }
}

}

// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstddef>

#include <windows.h>

#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/protect.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/type_traits.hpp>
#include <hadesmem/detail/query_region.hpp>
#include <hadesmem/detail/protect_guard.hpp>

namespace hadesmem
{

class Process;

namespace detail
{

inline void WriteUnchecked(Process const& process, PVOID address, 
  LPCVOID data, std::size_t len)
{
  HADESMEM_ASSERT(address != nullptr);
  HADESMEM_ASSERT(data != nullptr);
  HADESMEM_ASSERT(len != 0);

  SIZE_T bytes_written = 0;
  if (!::WriteProcessMemory(process.GetHandle(), address, data, 
    len, &bytes_written) || bytes_written != len)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Could not write process memory.") << 
      ErrorCodeWinLast(last_error));
  }
}

inline void Write(Process const& process, PVOID address, LPCVOID data, 
  std::size_t len)
{
  HADESMEM_ASSERT(address != nullptr);
  HADESMEM_ASSERT(data != nullptr);
  HADESMEM_ASSERT(len != 0);

  ProtectGuard protect_guard(process, address, ProtectGuardType::kWrite);
  
  WriteUnchecked(process, address, data, len);

  protect_guard.Restore();
}

}

}

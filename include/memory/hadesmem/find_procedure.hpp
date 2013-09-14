// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <string>

#include <windows.h>

#include <hadesmem/module.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/detail/find_procedure.hpp>

namespace hadesmem
{
  
inline FARPROC FindProcedure(
  Process const& process, 
  Module const& module, 
  std::string const& name)
{
  FARPROC const remote_func = detail::GetProcAddressInternal(
    process, module.GetHandle(), name);
  if (!remote_func)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error() << 
      ErrorString("GetProcAddressInternal failed.") << 
      ErrorCodeWinLast(last_error));
  }

  return remote_func;
}

inline FARPROC FindProcedure(
  Process const& process, 
  Module const& module, 
  WORD ordinal)
{
  FARPROC const remote_func = detail::GetProcAddressInternal(
    process, module.GetHandle(), ordinal);
  if (!remote_func)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error() << 
      ErrorString("GetProcAddressInternal failed.") << 
      ErrorCodeWinLast(last_error));
  }

  return remote_func;
}

}

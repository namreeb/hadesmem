// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <string>

#include <windows.h>

#include <hadesmem/detail/find_procedure.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/process.hpp>

namespace hadesmem
{
inline FARPROC FindProcedure(Process const& process,
                             Module const& module,
                             std::string const& name)
{
  FARPROC const remote_func =
    detail::GetProcAddressInternal(process, module.GetHandle(), name);
  if (!remote_func)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error{} << ErrorString{"GetProcAddressInternal failed."});
  }

  return remote_func;
}

inline FARPROC
  FindProcedure(Process const& process, Module const& module, WORD ordinal)
{
  FARPROC const remote_func =
    detail::GetProcAddressInternal(process, module.GetHandle(), ordinal);
  if (!remote_func)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error{} << ErrorString{"GetProcAddressInternal failed."});
  }

  return remote_func;
}
}

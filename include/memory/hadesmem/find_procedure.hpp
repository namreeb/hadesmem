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
  return detail::FindProcedureInternal(
    process, 
    module.GetHandle(), 
    name.c_str());
}

inline FARPROC FindProcedure(
  Process const& process, 
  Module const& module, 
  WORD ordinal)
{
  return detail::FindProcedureInternal(
    process, 
    module.GetHandle(), 
    MAKEINTRESOURCEA(ordinal));
}

}

// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/detail/module_find_procedure.hpp"

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/assert.hpp>
#include <boost/scope_exit.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/error.hpp"
#include "hadesmem/module.hpp"

namespace hadesmem
{

namespace detail
{

FARPROC FindProcedureInternal(Module const& module, LPCSTR name)
{
  HMODULE const local_module(LoadLibraryEx(module.GetPath().c_str(), nullptr, 
    DONT_RESOLVE_DLL_REFERENCES));
  if (!local_module)
  {
    DWORD const last_error = GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Could not load module locally.") << 
      ErrorCodeWinLast(last_error));
  }
  
  BOOST_SCOPE_EXIT_ALL(&)
  {
    // WARNING: Handle is leaked if FreeLibrary fails.
    BOOST_VERIFY(FreeLibrary(local_module));
  };
  
  FARPROC const local_func = GetProcAddress(local_module, name);
  if (!local_func)
  {
    DWORD const last_error = GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Could not find target function.") << 
      ErrorCodeWinLast(last_error));
  }
  
  LONG_PTR const func_delta = reinterpret_cast<DWORD_PTR>(local_func) - 
    reinterpret_cast<DWORD_PTR>(static_cast<HMODULE>(local_module));
  
  FARPROC const remote_func = reinterpret_cast<FARPROC>(
    reinterpret_cast<DWORD_PTR>(module.GetHandle()) + func_delta);
  
  return remote_func;
}

}

}

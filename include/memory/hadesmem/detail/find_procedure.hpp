// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <string>

#include <windows.h>

#include <hadesmem/error.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/pelib/export.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/detail/str_conv.hpp>
#include <hadesmem/pelib/export_list.hpp>
#include <hadesmem/detail/static_assert.hpp>

namespace hadesmem
{

namespace detail
{

// TODO: Support exports by ordinal, add a new overload and change 
// FindProcedure to use it.
inline FARPROC GetProcAddressInternal(
  Process const& process, 
  HMODULE const& module, 
  std::string const& export_name)
{
  HADESMEM_DETAIL_STATIC_ASSERT(sizeof(FARPROC) == sizeof(void*));

  PeFile const pe_file(process, module, PeFileType::Image);
  
  ExportList const exports(process, pe_file);
  for (auto const& e : exports)
  {
    if (e.ByName() && e.GetName() == export_name)
    {
      if (e.IsForwarded())
      {
        std::string const forwarder_module_name(e.GetForwarderModule());
        Module const forwarder_module(process, 
          MultiByteToWideChar(forwarder_module_name));
        return GetProcAddressInternal(
          process, 
          forwarder_module.GetHandle(), 
          e.GetForwarderFunction());
      }

      // TODO: Investigate whether there is a way to do this that isn't 
      // illegal. Or at least, a way that's potentially friendlier to the 
      // compiler. Perhaps a union? What do compilers prefer?
      void* va = e.GetVa();
      FARPROC pfn = nullptr;
      std::memcpy(&pfn, &va, sizeof(void*));

      return pfn;
    }
  }

  return nullptr;
}

inline FARPROC FindProcedureInternal(
  Process const& process, 
  HMODULE module, 
  LPCSTR name)
{
  HADESMEM_DETAIL_ASSERT(name != nullptr);

  FARPROC const remote_func = GetProcAddressInternal(
    process, module, name);
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

}

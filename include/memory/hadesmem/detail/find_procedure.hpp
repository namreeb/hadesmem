// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <algorithm>
#include <string>

#include <windows.h>

#include <hadesmem/pelib/export_list.hpp>
#include <hadesmem/detail/static_assert.hpp>
#include <hadesmem/detail/str_conv.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/pelib/export.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>

namespace hadesmem
{

namespace detail
{
  
inline FARPROC GetProcAddressFromExport(
  Process const& process, 
  Export const& e);

template <typename Pred>
inline FARPROC GetProcAddressInternalFromPred(
  Process const& process, 
  HMODULE const& module, 
  Pred pred)
{
  HADESMEM_DETAIL_STATIC_ASSERT(sizeof(FARPROC) == sizeof(void*));

  PeFile const pe_file(process, module, PeFileType::Image);
  
  ExportList const exports(process, pe_file);
  auto const iter = std::find_if(
    std::begin(exports), 
    std::end(exports), 
    pred);
  if (iter != std::end(exports))
  {
    return GetProcAddressFromExport(process, *iter);
  }

  return nullptr;
}

inline FARPROC GetProcAddressInternal(
  Process const& process, 
  HMODULE module, 
  std::string const& name)
{
  auto const pred = 
    [&] (Export const& e)
  {
    return e.ByName() && e.GetName() == name;
  };
  return GetProcAddressInternalFromPred(process, module, pred);
}

inline FARPROC GetProcAddressInternal(
  Process const& process, 
  HMODULE module, 
  WORD ordinal)
{
  auto const pred = 
    [&] (Export const& e)
  {
    return e.ByOrdinal() && e.GetOrdinal() == ordinal;
  };
  return GetProcAddressInternalFromPred(process, module, pred);
}

inline FARPROC GetProcAddressFromExport(
  Process const& process, 
  Export const& e)
{
  if (e.IsForwarded())
  {
    std::string const forwarder_module_name(e.GetForwarderModule());
    Module const forwarder_module(
      process, 
      MultiByteToWideChar(forwarder_module_name));
    if (e.IsForwardedByOrdinal())
    {
      return GetProcAddressInternal(
        process, 
        forwarder_module.GetHandle(), 
        e.GetForwarderOrdinal());
    }
    else
    {
      return GetProcAddressInternal(
        process, 
        forwarder_module.GetHandle(), 
        e.GetForwarderFunction());
    }
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

}

// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/error.hpp>

namespace hadesmem
{

namespace detail
{

inline HMODULE GetHandleToSelf()
{
  auto const this_func_ptr = reinterpret_cast<void const*>(
    reinterpret_cast<std::uintptr_t>(&GetHandleToSelf));
  HMODULE handle{};
  if (!::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                              GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            static_cast<wchar_t const*>(this_func_ptr),
                            &handle))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"VirtualQuery failed."}
                                    << ErrorCodeWinLast{last_error});
  }

  return handle;
}

inline std::wstring GetSelfPath()
{
  std::vector<wchar_t> path(HADESMEM_DETAIL_MAX_PATH_UNICODE);
  DWORD const path_out_len = ::GetModuleFileNameW(
    GetHandleToSelf(), path.data(), static_cast<DWORD>(path.size()));
  if (!path_out_len || ::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"GetModuleFileName failed."}
                                    << ErrorCodeWinLast{last_error});
  }

  return path.data();
}

inline std::wstring GetSelfDirPath()
{
  std::wstring self_path = GetSelfPath();
  std::wstring::size_type const separator = self_path.rfind(L'\\');
  HADESMEM_DETAIL_ASSERT(separator != std::wstring::npos);
  self_path.erase(separator);
  return self_path;
}
}
}

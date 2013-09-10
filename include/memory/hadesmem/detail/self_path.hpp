// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <string>
#include <vector>

#include <windows.h>

#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/detail/assert.hpp>

namespace hadesmem
{

namespace detail
{

inline HMODULE GetHandleToSelf()
{
  MEMORY_BASIC_INFORMATION mem_info;
  ZeroMemory(&mem_info, sizeof(mem_info));
  auto const this_func_ptr = reinterpret_cast<LPCVOID>(
    reinterpret_cast<DWORD_PTR>(&GetHandleToSelf));
  if (!::VirtualQuery(this_func_ptr, &mem_info, sizeof(mem_info)))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error() << 
      ErrorString("VirtualQuery failed.") << 
      ErrorCodeWinLast(last_error));
  }

  return static_cast<HMODULE>(mem_info.AllocationBase);
}

inline std::wstring GetSelfPath()
{
  std::vector<wchar_t> path(HADESMEM_DETAIL_MAX_PATH_UNICODE);
  DWORD const path_out_len = ::GetModuleFileName(GetHandleToSelf(), 
    path.data(), static_cast<DWORD>(path.size()));
  if (!path_out_len || ::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error() << 
      ErrorString("GetModuleFileName failed.") << 
      ErrorCodeWinLast(last_error));
  }

  return path.data();
}

inline std::wstring GetSelfDirPath()
{
  std::wstring self_path(GetSelfPath());
  std::wstring::size_type const separator = self_path.rfind(L'\\');
  HADESMEM_DETAIL_ASSERT(separator != std::wstring::npos);
  self_path.erase(separator);
  return self_path;
}

}

}

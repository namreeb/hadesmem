// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/detail/self_path.hpp"

#include <vector>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/assert.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include "hadesmem/error.hpp"
#include "hadesmem/config.hpp"

namespace hadesmem
{

namespace detail
{

HMODULE GetHandleToSelf()
{
  MEMORY_BASIC_INFORMATION mem_info;
  ZeroMemory(&mem_info, sizeof(mem_info));
  auto const this_func_ptr = reinterpret_cast<LPCVOID>(
    reinterpret_cast<DWORD_PTR>(&GetHandleToSelf));
  if (!::VirtualQuery(this_func_ptr, &mem_info, sizeof(mem_info)))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Failed to query memory.") << 
      ErrorCodeWinLast(last_error));
  }

  return static_cast<HMODULE>(mem_info.AllocationBase);
}

std::wstring GetSelfPath()
{
  std::vector<wchar_t> path(HADESMEM_MAX_PATH_UNICODE);
  DWORD const path_out_len = ::GetModuleFileName(GetHandleToSelf(), 
    path.data(), static_cast<DWORD>(path.size()));
  if (!path_out_len || ::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("GetModuleFileName failed.") << 
      ErrorCodeWinLast(last_error));
  }

  return path.data();
}

std::wstring GetSelfDirPath()
{
  std::wstring self_path(GetSelfPath());
  std::wstring::size_type const separator = self_path.rfind(L'\\');
  BOOST_ASSERT(separator != std::wstring::npos);
  self_path.erase(separator);
  return self_path;
}

}

}

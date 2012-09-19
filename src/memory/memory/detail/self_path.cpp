// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/detail/self_path.hpp"

#include <array>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/filesystem.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/error.hpp"

namespace hadesmem
{

namespace detail
{

HMODULE GetHandleToSelf()
{
  MEMORY_BASIC_INFORMATION mem_info;
  ZeroMemory(&mem_info, sizeof(mem_info));
  PVOID this_func_ptr = reinterpret_cast<PVOID>(
    reinterpret_cast<DWORD_PTR>(&GetHandleToSelf));
  if (!::VirtualQuery(this_func_ptr, &mem_info, sizeof(mem_info)))
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Failed to query memory.") << 
      ErrorCodeWinLast(last_error));
  }

  return reinterpret_cast<HMODULE>(mem_info.AllocationBase);
}

std::wstring GetSelfPath()
{
  std::array<wchar_t, MAX_PATH> path = { { 0 } };
  DWORD const path_out_len = ::GetModuleFileName(GetHandleToSelf(), 
    path.data(), static_cast<DWORD>(path.size()));
  if (!path_out_len || ::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("GetModuleFileName failed.") << 
      ErrorCodeWinLast(last_error));
  }

  return path.data();
}

std::wstring GetSelfDirPath()
{
  boost::filesystem::path const path(GetSelfPath());
  return path.parent_path().native();
}

}

}

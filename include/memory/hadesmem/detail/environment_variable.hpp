// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <string>
#include <utility>
#include <vector>

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>

namespace hadesmem
{
namespace detail
{
std::pair<bool, std::vector<wchar_t>>
  ReadEnvironmentVariable(std::wstring const& name)
{
  std::pair<bool, std::vector<wchar_t>> value{
    false, std::vector<wchar_t>(HADESMEM_DETAIL_MAX_PATH_UNICODE)};
  auto const err = ::GetEnvironmentVariableW(
    name.c_str(), value.second.data(), static_cast<DWORD>(value.second.size()));
  if (!err || err > value.second.size())
  {
    DWORD const last_error = ::GetLastError();
    if (last_error != ERROR_ENVVAR_NOT_FOUND)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error{} << ErrorString{"GetEnvironmentVariableW failed."}
                << ErrorCodeWinLast{last_error});
    }
  }
  else
  {
    value.first = true;
  }

  return value;
}

void WriteEnvironmentVariable(std::wstring const& name, wchar_t const* data)
{
  if (!::SetEnvironmentVariableW(name.c_str(), data))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error{} << ErrorString{"SetEnvironmentVariableW failed."}
              << ErrorCodeWinLast{last_error});
  }
}
}
}

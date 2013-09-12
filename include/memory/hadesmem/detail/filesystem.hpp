// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>
#include <shlwapi.h>

#include <hadesmem/config.hpp>

namespace hadesmem
{

namespace detail
{

inline bool DoesFileExist(std::wstring const& path)
{
  return ::GetFileAttributes(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

inline bool IsPathRelative(std::wstring const& path)
{
  return ::PathIsRelative(path.c_str()) != FALSE;
}

inline std::wstring CombinePath(std::wstring const& base, std::wstring const& append)
{
  std::vector<wchar_t> buffer(MAX_PATH);
  if (!::PathCombineW(buffer.data(), base.c_str(), append.c_str()))
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(Error() << 
      ErrorString("PathCombineW failed.") << 
      ErrorCodeWinLast(last_error));
  }

  return buffer.data();
}

}

}

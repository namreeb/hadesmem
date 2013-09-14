// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <string>
#include <vector>

#include <windows.h>

#include <hadesmem/error.hpp>

namespace hadesmem
{

namespace detail
{

inline std::string WideCharToMultiByte(std::wstring const& in)
{
  int const buf_len = ::WideCharToMultiByte(
    CP_OEMCP, 
    WC_ERR_INVALID_CHARS | WC_NO_BEST_FIT_CHARS, 
    in.c_str(), 
    -1, 
    nullptr, 
    0, 
    nullptr, 
    nullptr);
  if (!buf_len)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error() << 
      ErrorString("WideCharToMultiByte failed.") << 
      ErrorCodeWinLast(last_error));
  }

  // TODO: Bounds checking before static_cast.
  std::vector<char> buf(static_cast<std::size_t>(buf_len));
  if (!::WideCharToMultiByte(
    CP_OEMCP, 
    WC_ERR_INVALID_CHARS | WC_NO_BEST_FIT_CHARS, 
    in.c_str(), 
    -1, 
    buf.data(), 
    buf_len, 
    nullptr, 
    nullptr))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error() << 
      ErrorString("WideCharToMultiByte failed.") << 
      ErrorCodeWinLast(last_error));
  }

  return buf.data();
}

inline std::wstring MultiByteToWideChar(std::string const& in)
{
  int const buf_len = ::MultiByteToWideChar(
    CP_OEMCP, 
    MB_ERR_INVALID_CHARS, 
    in.c_str(), 
    -1, 
    nullptr, 
    0);
  if (!buf_len)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error() << 
      ErrorString("MultiByteToWideChar failed.") << 
      ErrorCodeWinLast(last_error));
  }

  // TODO: Bounds checking before static_cast.
  std::vector<wchar_t> buf(static_cast<std::size_t>(buf_len));
  if (!::MultiByteToWideChar(
    CP_OEMCP, 
    MB_ERR_INVALID_CHARS, 
    in.c_str(), 
    -1, 
    buf.data(), 
    buf_len))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error() << 
      ErrorString("MultiByteToWideChar failed.") << 
      ErrorCodeWinLast(last_error));
  }

  return buf.data();
}

}

}

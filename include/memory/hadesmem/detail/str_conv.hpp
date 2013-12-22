// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <limits>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

#include <windows.h>

#include <hadesmem/error.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/static_assert.hpp>

namespace hadesmem
{

namespace detail
{

// String must be hex.
inline std::uintptr_t HexStrToPtr(std::wstring const& str)
{
  std::wstringstream ss(str);
  ss.imbue(std::locale::classic());
  std::uintptr_t ptr = 0;
  if (!(ss >> std::hex >> ptr))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error() << ErrorString("String to pointer conversion failed."));
  }
  return ptr;
}

template <typename T, typename CharT>
T StrToNum(std::basic_string<CharT> const& str)
{
  HADESMEM_DETAIL_STATIC_ASSERT(std::is_integral<T>::value);
  std::basic_ostringstream<CharT> converter(str);
  converter.imbue(std::locale::classic());
  T out = 0;
  if (!converter || (!converter >> out))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                    << ErrorString("Conversion failed."));
  }
  return out;
}

inline std::string WideCharToMultiByte(std::wstring const& in)
{
  std::int32_t const buf_len =
    ::WideCharToMultiByte(CP_OEMCP,
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
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error() << ErrorString("WideCharToMultiByte failed.")
              << ErrorCodeWinLast(last_error));
  }
  HADESMEM_DETAIL_ASSERT(buf_len > 0);

  std::vector<char> buf(static_cast<std::size_t>(buf_len));
  if (!::WideCharToMultiByte(CP_OEMCP,
                             WC_ERR_INVALID_CHARS | WC_NO_BEST_FIT_CHARS,
                             in.c_str(),
                             -1,
                             buf.data(),
                             buf_len,
                             nullptr,
                             nullptr))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error() << ErrorString("WideCharToMultiByte failed.")
              << ErrorCodeWinLast(last_error));
  }

  return buf.data();
}

inline std::wstring MultiByteToWideChar(std::string const& in)
{
  std::int32_t const buf_len = ::MultiByteToWideChar(
    CP_OEMCP, MB_ERR_INVALID_CHARS, in.c_str(), -1, nullptr, 0);
  if (!buf_len)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error() << ErrorString("MultiByteToWideChar failed.")
              << ErrorCodeWinLast(last_error));
  }
  HADESMEM_DETAIL_ASSERT(buf_len > 0);

  std::vector<wchar_t> buf(static_cast<std::size_t>(buf_len));
  if (!::MultiByteToWideChar(
         CP_OEMCP, MB_ERR_INVALID_CHARS, in.c_str(), -1, buf.data(), buf_len))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error() << ErrorString("MultiByteToWideChar failed.")
              << ErrorCodeWinLast(last_error));
  }

  return buf.data();
}
}
}

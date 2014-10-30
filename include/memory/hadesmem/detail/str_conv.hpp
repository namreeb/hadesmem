// Copyright (C) 2010-2014 Joshua Boyce.
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
  std::wstringstream ss{str};
  ss.imbue(std::locale::classic());
  std::uintptr_t ptr = 0;
  if (!(ss >> std::hex >> ptr))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error{} << ErrorString{"String to pointer conversion failed."});
  }
  return ptr;
}

inline std::wstring PtrToHexString(void const* const ptr)
{
  std::wostringstream ss;
  ss.imbue(std::locale::classic());
  if (!(ss << std::hex << reinterpret_cast<std::uintptr_t>(ptr)))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error{} << ErrorString{"Pointer to string conversion failed."});
  }
  return ss.str();
}

template <typename T, typename CharT>
T StrToNum(std::basic_string<CharT> const& str)
{
  HADESMEM_DETAIL_STATIC_ASSERT(std::is_integral<T>::value);
  std::basic_istringstream<CharT> converter{str};
  converter.imbue(std::locale::classic());
  T out;
  if (!converter || !(converter >> out))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"Conversion failed."});
  }
  return out;
}

template <typename CharT, typename T> std::basic_string<CharT> NumToStr(T num)
{
  HADESMEM_DETAIL_STATIC_ASSERT(std::is_integral<T>::value);
  std::basic_ostringstream<CharT> converter;
  converter.imbue(std::locale::classic());
  if (!converter || !(converter << num))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"Conversion failed."});
  }
  return converter.str();
}

inline std::string WideCharToMultiByte(std::string const& in,
                                       bool* lossy = nullptr)
{
  if (lossy)
  {
    *lossy = false;
  }

  return in;
}

inline std::string WideCharToMultiByte(std::wstring const& in,
                                       bool* lossy = nullptr)
{
  std::int32_t const buf_len = ::WideCharToMultiByte(CP_OEMCP,
                                                     WC_NO_BEST_FIT_CHARS,
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
      Error{} << ErrorString{"WideCharToMultiByte failed."}
              << ErrorCodeWinLast{last_error});
  }
  HADESMEM_DETAIL_ASSERT(buf_len > 0);

  std::vector<char> buf(static_cast<std::size_t>(buf_len));
  BOOL lossy_tmp = FALSE;
  if (!::WideCharToMultiByte(CP_OEMCP,
                             WC_NO_BEST_FIT_CHARS,
                             in.c_str(),
                             -1,
                             buf.data(),
                             buf_len,
                             nullptr,
                             &lossy_tmp))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error{} << ErrorString{"WideCharToMultiByte failed."}
              << ErrorCodeWinLast{last_error});
  }

  if (lossy)
  {
    *lossy = !!lossy_tmp;
  }

  return buf.data();
}

inline std::wstring MultiByteToWideChar(std::wstring const& in)
{
  return in;
}

inline std::wstring MultiByteToWideChar(std::string const& in)
{
  std::int32_t const buf_len = ::MultiByteToWideChar(
    CP_OEMCP, MB_ERR_INVALID_CHARS, in.c_str(), -1, nullptr, 0);
  if (!buf_len)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error{} << ErrorString{"MultiByteToWideChar failed."}
              << ErrorCodeWinLast{last_error});
  }
  HADESMEM_DETAIL_ASSERT(buf_len > 0);

  std::vector<wchar_t> buf(static_cast<std::size_t>(buf_len));
  if (!::MultiByteToWideChar(
        CP_OEMCP, MB_ERR_INVALID_CHARS, in.c_str(), -1, buf.data(), buf_len))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error{} << ErrorString{"MultiByteToWideChar failed."}
              << ErrorCodeWinLast{last_error});
  }

  return buf.data();
}
}
}

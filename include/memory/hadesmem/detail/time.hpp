// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <array>
#include <ctime>
#include <cwchar>
#include <string>
#include <time.h>

#include <windows.h>

#include <hadesmem/error.hpp>

namespace hadesmem
{
namespace detail
{
inline std::wstring TimestampToStringUtc(std::time_t time)
{
  std::tm utc_time{};
  auto const utc_time_conv = gmtime_s(&utc_time, &time);
  if (utc_time_conv)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error() << hadesmem::ErrorString("Invalid timestamp."));
  }

  std::array<wchar_t, _countof(L"YYYYMMDD-HHMMSS")> fmt_buf{};
  if (!std::wcsftime(
        fmt_buf.data(), fmt_buf.size(), L"%Y%m%d-%H%M%S", &utc_time))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error() << hadesmem::ErrorString("Invalid timestamp."));
  }

  return fmt_buf.data();
}
}
}

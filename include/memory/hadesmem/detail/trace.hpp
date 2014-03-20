// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <cstdio>
#include <sstream>
#include <utility>
#include <vector>

#include <windows.h>

#include <hadesmem/detail/assert.hpp>

namespace hadesmem
{

namespace detail
{

inline void OutputDebugString(char const* const s)
{
  ::OutputDebugStringA(s);
}

inline void OutputDebugString(wchar_t const* const s)
{
  ::OutputDebugStringW(s);
}
}
}

#if !defined(HADESMEM_NO_TRACE)

#define HADESMEM_DETAIL_TRACE_MULTI_LINE_MACRO_BEGIN                           \
                                                                               \
  do                                                                           \
  \
{

#define HADESMEM_DETAIL_TRACE_MULTI_LINE_MACRO_END                             \
  \
}                                                                         \
  \
while((void)0, 0)

#define HADESMEM_DETAIL_TRACE_RAW(x) ::hadesmem::detail::OutputDebugString(x)

template <typename CharT, typename FuncT, typename FormatT, typename... Args>
void TraceFormatImpl(FuncT func, FormatT format, Args&&... args)
{
  std::int32_t const num_char =
    func(nullptr, 0, format, std::forward<Args>(args)...);
  HADESMEM_DETAIL_ASSERT(num_char > 0);
  if (num_char > 0)
  {
    std::vector<CharT> trace_buffer(static_cast<std::size_t>(num_char + 1));
    std::int32_t const num_char_actual =
      func(trace_buffer.data(),
           static_cast<std::size_t>(num_char),
           format,
           std::forward<Args>(args)...);
    HADESMEM_DETAIL_ASSERT(num_char_actual > 0);
    (void)num_char_actual;
    std::wstringstream formatter;
    formatter << __FUNCTION__ << ": " << trace_buffer.data() << "\n";
    HADESMEM_DETAIL_TRACE_RAW(formatter.str().c_str());
  }
}

#define HADESMEM_DETAIL_TRACE_FORMAT_IMPL(                                     \
  detail_char_type, detail_format_func, detail_format, ...)                    \
                                                                               \
  HADESMEM_DETAIL_TRACE_MULTI_LINE_MACRO_BEGIN TraceFormatImpl<                \
    detail_char_type>(detail_format_func, detail_format, __VA_ARGS__);         \
  HADESMEM_DETAIL_TRACE_MULTI_LINE_MACRO_END

#define HADESMEM_DETAIL_TRACE_FORMAT_A(format, ...)                            \
  HADESMEM_DETAIL_TRACE_FORMAT_IMPL(char, _snprintf, format, __VA_ARGS__)

#define HADESMEM_DETAIL_TRACE_FORMAT_W(format, ...)                            \
  HADESMEM_DETAIL_TRACE_FORMAT_IMPL(wchar_t, _snwprintf, format, __VA_ARGS__)

#define HADESMEM_DETAIL_TRACE_A(x) HADESMEM_DETAIL_TRACE_FORMAT_A("%s", x)

#define HADESMEM_DETAIL_TRACE_W(x) HADESMEM_DETAIL_TRACE_FORMAT_W(L"%s", x)

#else // #if !defined(HADESMEM_NO_TRACE)

#define HADESMEM_DETAIL_TRACE_RAW(x)
#define HADESMEM_DETAIL_TRACE_A(x)
#define HADESMEM_DETAIL_TRACE_FORMAT_A(...)
#define HADESMEM_DETAIL_TRACE_W(x)
#define HADESMEM_DETAIL_TRACE_FORMAT_W(...)

#endif // #if !defined(HADESMEM_NO_TRACE)

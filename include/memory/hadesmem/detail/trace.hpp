// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <vector>
#include <cstdio>

#include <windows.h>

#include <hadesmem/detail/assert.hpp>

#if !defined(HADESMEM_NO_TRACE)

#define HADESMEM_TRACE_MULTI_LINE_MACRO_BEGIN \
do\
{
#define HADESMEM_TRACE_MULTI_LINE_MACRO_END \
} while((void)0,0)

#define HADESMEM_TRACE_RAW_A(x) ::OutputDebugStringA(x)

#define HADESMEM_TRACE_FORMAT_A(format, ...) \
HADESMEM_TRACE_MULTI_LINE_MACRO_BEGIN \
std::vector<char> trace_buffer;\
int const num_char = _snprintf(trace_buffer.data(), 0, format, __VA_ARGS__);\
HADESMEM_DETAIL_ASSERT(num_char > 0);\
if (num_char > 0)\
{\
  trace_buffer.resize(static_cast<std::size_t>(num_char + 1));\
  int const num_char_actual = _snprintf(trace_buffer.data(), static_cast<std::size_t>(num_char), format, __VA_ARGS__);\
  HADESMEM_DETAIL_ASSERT(num_char_actual > 0);\
  (void)num_char_actual;\
  HADESMEM_TRACE_RAW_A(__FUNCTION__);\
  HADESMEM_TRACE_RAW_A(": ");\
  HADESMEM_TRACE_RAW_A(trace_buffer.data());\
  HADESMEM_TRACE_RAW_A("\n");\
}\
HADESMEM_TRACE_MULTI_LINE_MACRO_END

#define HADESMEM_TRACE_A(x) HADESMEM_TRACE_FORMAT_A("%s", x)

#define HADESMEM_TRACE_RAW_W(x) ::OutputDebugStringW(x)

#define HADESMEM_TRACE_FORMAT_W(format, ...) \
HADESMEM_TRACE_MULTI_LINE_MACRO_BEGIN \
std::vector<wchar_t> trace_buffer;\
int const num_char = _snwprintf(trace_buffer.data(), 0, format, __VA_ARGS__);\
HADESMEM_DETAIL_ASSERT(num_char > 0);\
if (num_char > 0)\
{\
  trace_buffer.resize(static_cast<std::size_t>(num_char + 1));\
  int const num_char_actual = _snwprintf(trace_buffer.data(), static_cast<std::size_t>(num_char), format, __VA_ARGS__);\
  HADESMEM_DETAIL_ASSERT(num_char_actual > 0);\
  (void)num_char_actual;\
  HADESMEM_TRACE_RAW_A(__FUNCTION__);\
  HADESMEM_TRACE_RAW_A(": ");\
  HADESMEM_TRACE_RAW_W(trace_buffer.data());\
  HADESMEM_TRACE_RAW_A("\n");\
}\
HADESMEM_TRACE_MULTI_LINE_MACRO_END

#define HADESMEM_TRACE_W(x) HADESMEM_TRACE_FORMAT_W("%s", x)

#else
#define HADESMEM_TRACE_RAW_A(x) 
#define HADESMEM_TRACE_A(x) 
#define HADESMEM_TRACE_FORMAT_A(format, ...) 
#define HADESMEM_TRACE_RAW_W(x) 
#define HADESMEM_TRACE_W(x) 
#define HADESMEM_TRACE_FORMAT_W(format, ...) 
#endif

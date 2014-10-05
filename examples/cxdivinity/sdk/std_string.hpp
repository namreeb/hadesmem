// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <string>

#include "static_assert.hpp"

namespace divinity
{

template <typename CharT> union StdStringBufT
{
  CharT buf_[0x10 / sizeof(CharT)];
  CharT* ptr_;
  char pad_[0x10];
};

using StdStringBufA = StdStringBufT<char>;

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(StdStringBufA) == 0x10);

using StdStringBufW = StdStringBufT<wchar_t>;

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(StdStringBufW) == 0x10);

template <typename CharT> struct StdStringT
{
  void* alloc_;
  StdStringBufT<CharT> storage_;
  unsigned int size_;
  unsigned int capacity_;
};

using StdStringA = StdStringT<char>;

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(StdStringA) == 0x1C);

using StdStringW = StdStringT<wchar_t>;

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(StdStringW) == 0x1C);
}

template <typename CharT>
CharT* GetStdStringRaw(divinity::StdStringT<CharT>* string)
{
  return (string->capacity_ < (0x10 / sizeof(CharT))) ? string->storage_.buf_
                                                      : string->storage_.ptr_;
}

template <typename CharT>
std::basic_string<CharT> ConvertStdString(divinity::StdStringT<CharT>* string)
{
  return GetStdStringRaw(string);
}

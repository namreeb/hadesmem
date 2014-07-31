// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstddef>
#include <string>

#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>

template <typename CharT> struct StdString
{
  union Buffer
  {
    CharT buf_[0x10 / sizeof(CharT)];
    CharT* ptr_;
    char alias_[0x10];
  };

  void* alloc_;
  Buffer storage_;
  std::size_t size_;
  std::size_t capacity_;
};

typedef StdString<char> StdStringA;

typedef StdString<wchar_t> StdStringW;

template <typename CharT>
std::basic_string<CharT> GetString(hadesmem::Process const& process,
                                   StdString<CharT> const& str)
{
  if (str.capacity_ < 0x10 / sizeof(CharT))
  {
    return str.storage_.buf_;
  }
  else
  {
    return hadesmem::ReadString<CharT>(process, str.storage_.ptr_);
  }
}

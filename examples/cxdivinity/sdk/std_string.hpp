// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include "static_assert.hpp"

namespace divinity
{

union StdStringABuf
{
  char buf_[16];
  char* ptr_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(StdStringABuf) == 0x10);

union StdStringWBuf
{
  wchar_t buf_[8];
  wchar_t* ptr_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(StdStringWBuf) == 0x10);

struct StdStringA
{
  void* alloc_;
  StdStringABuf storage_;
  unsigned int size_;
  unsigned int capacity_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(StdStringA) == 0x1C);

struct StdStringW
{
  void* alloc_;
  StdStringWBuf storage_;
  unsigned int size_;
  unsigned int capacity_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(StdStringW) == 0x1C);
}

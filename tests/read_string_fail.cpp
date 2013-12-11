// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/read.hpp>

#include <hadesmem/process.hpp>

void TestReadStringFail()
{
  hadesmem::Process const process(::GetCurrentProcessId());

  hadesmem::ReadString<int>(process, nullptr);
}

int main()
{
  TestReadStringFail();
}

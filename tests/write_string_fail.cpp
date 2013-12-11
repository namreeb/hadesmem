// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/write.hpp>

#include <hadesmem/process.hpp>

struct non_string_type
{
};

void TestWriteStringFail()
{
  hadesmem::Process const process(::GetCurrentProcessId());

  hadesmem::WriteString(process, nullptr, non_string_type());
}

int main()
{
  TestWriteStringFail();
}

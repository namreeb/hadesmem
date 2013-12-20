// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/read.hpp>
#include <hadesmem/read.hpp>

#include <hadesmem/process.hpp>

struct NotDefaultConstructible
{
  NotDefaultConstructible(int)
  {
  }
};

void TestReadVectorFail()
{
  hadesmem::Process const process(::GetCurrentProcessId());

  hadesmem::ReadVector<NotDefaultConstructible>(process, nullptr, 1);
}

int main()
{
  TestReadVectorFail();
}

// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/write.hpp>
#include <hadesmem/write.hpp>

#include <hadesmem/process.hpp>

struct NonVectorType
{
};

void TestWriteVectorFail()
{
  hadesmem::Process const process(::GetCurrentProcessId());

  hadesmem::WriteVector(process, nullptr, NonVectorType());
}

int main()
{
  TestWriteVectorFail();
}

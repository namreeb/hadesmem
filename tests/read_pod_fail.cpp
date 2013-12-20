// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/read.hpp>
#include <hadesmem/read.hpp>

#include <hadesmem/process.hpp>

struct non_pod_type
{
  virtual void foo()
  {
  }

  virtual ~non_pod_type()
  {
  }
};

void TestReadPodFail()
{
  hadesmem::Process const process(::GetCurrentProcessId());

  hadesmem::Read<non_pod_type>(process, nullptr);
}

int main()
{
  TestReadPodFail();
}

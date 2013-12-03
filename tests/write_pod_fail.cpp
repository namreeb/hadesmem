// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/write.hpp>

#include <hadesmem/process.hpp>

struct non_pod_type
{
    virtual ~non_pod_type() {}
};

void TestWritePodFail()
{
    hadesmem::Process const process(::GetCurrentProcessId());

    hadesmem::Write(process, nullptr, non_pod_type());
}

int main()
{
    TestWritePodFail();
}

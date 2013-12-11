// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <array>

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/remote_thread.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/write.hpp>

namespace hadesmem
{

namespace detail
{

inline void ForceLdrInitializeThunk(DWORD proc_id)
{
  Process const process(proc_id);

// This is used to generate a 'nullsub' function, which is called
// in the context of the remote process in order to 'force' a
// call to ntdll.dll!LdrInitializeThunk. This is necessary
// because module enumeration will fail if LdrInitializeThunk has
// not been called, and Injector::InjectDll (and the APIs it
// uses) depends on the module enumeration APIs.
#if defined(HADESMEM_DETAIL_ARCH_X64)
  std::array<BYTE, 1> const return_instr = {{0xC3}};
#elif defined(HADESMEM_DETAIL_ARCH_X86)
  std::array<BYTE, 3> const return_instr = {{0xC2, 0x04, 0x00}};
#else
#error "[HadesMem] Unsupported architecture."
#endif

  HADESMEM_DETAIL_TRACE_A("Allocating memory for remote stub.");

  Allocator const stub_remote(process, sizeof(return_instr));

  HADESMEM_DETAIL_TRACE_A("Writing remote stub.");

  Write(process, stub_remote.GetBase(), return_instr);

  auto const stub_remote_pfn = reinterpret_cast<LPTHREAD_START_ROUTINE>(
    reinterpret_cast<DWORD_PTR>(stub_remote.GetBase()));

  HADESMEM_DETAIL_TRACE_A("Starting remote thread.");

  // TODO: Configurable timeout. This will complicate resource
  // management however, as we will need to extend the lifetime of
  // the remote memory in case it executes after we time out. Also,
  // if it times out there is no way to try again in the future...
  // Should we just leak the memory on timeout? Return a 'future'
  // object? Some sort of combination? Requires more
  // investigation...
  CreateRemoteThreadAndWait(process, stub_remote_pfn);

  HADESMEM_DETAIL_TRACE_A("Remote thread complete.");
}
}
}

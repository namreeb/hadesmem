// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "main.hpp"

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/self_path.hpp>
#include <hadesmem/detail/region_alloc_size.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/thread.hpp>
#include <hadesmem/thread_entry.hpp>
#include <hadesmem/thread_helpers.hpp>
#include <hadesmem/thread_list.hpp>

#include "d3d11.hpp"
#include "file.hpp"
#include "module.hpp"
#include "process.hpp"

// WARNING! Most of this is untested, it's for expository and testing
// purposes only.

namespace
{

bool IsSafeToUnload()
{
  auto const& process = GetThisProcess();
  bool safe = true;
  std::size_t retries = 5;
  do
  {
    hadesmem::SuspendedProcess suspend{process.GetId()};
    hadesmem::ThreadList threads{process.GetId()};
    safe = true;
    for (auto const& thread_entry : threads)
    {
      auto const id = thread_entry.GetId();
      if (id == ::GetCurrentThreadId())
      {
        continue;
      }

      hadesmem::Thread const thread{id};
      auto const context = GetThreadContext(thread, CONTEXT_CONTROL);
#if defined(HADESMEM_DETAIL_ARCH_X64)
      auto const ip = reinterpret_cast<void const*>(context.Rip);
#elif defined(HADESMEM_DETAIL_ARCH_X86)
      auto const ip = reinterpret_cast<void const*>(context.Eip);
#else
#error "[HadesMem] Unsupported architecture."
#endif
      HADESMEM_DETAIL_ASSERT(ip);
      auto const this_module =
        reinterpret_cast<std::uint8_t*>(hadesmem::detail::GetHandleToSelf());
      auto const this_module_size = hadesmem::detail::GetRegionAllocSize(
        process, reinterpret_cast<void const*>(this_module));
      if (ip >= this_module && ip < this_module + this_module_size)
      {
        safe = false;
      }
    }
  } while (!safe && retries--);

  return safe;
}
}

hadesmem::Process& GetThisProcess()
{
  static hadesmem::Process process{::GetCurrentProcessId()};
  return process;
}

extern "C" HADESMEM_DETAIL_DLLEXPORT DWORD_PTR Load() HADESMEM_DETAIL_NOEXCEPT
{
  try
  {
    // Support deferred hooking
    InitializeD3D11();

    DetourNtQuerySystemInformation();
    DetourNtCreateUserProcess();
    DetourNtQueryDirectoryFile();
    DetourNtMapViewOfSection();
    DetourNtUnmapViewOfSection();

    DetourD3D11(nullptr);
    DetourDXGI(nullptr);

    return 0;
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);

    return 1;
  }
}

extern "C" HADESMEM_DETAIL_DLLEXPORT DWORD_PTR Free() HADESMEM_DETAIL_NOEXCEPT
{
  try
  {
    UndetourNtQuerySystemInformation();
    UndetourNtCreateUserProcess();
    UndetourNtQueryDirectoryFile();
    UndetourNtMapViewOfSection();
    UndetourNtUnmapViewOfSection();

    UndetourDXGI(true);
    UndetourD3D11(true);

    if (!IsSafeToUnload())
    {
      return 2;
    }

    return 0;
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);

    return 1;
  }
}

BOOL WINAPI
  DllMain(HINSTANCE /*instance*/, DWORD /*reason*/, LPVOID /*reserved*/)
  HADESMEM_DETAIL_NOEXCEPT
{
  return TRUE;
}

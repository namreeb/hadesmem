// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/thread.hpp>
#include <hadesmem/thread_entry.hpp>
#include <hadesmem/thread_helpers.hpp>

namespace hadesmem
{
namespace detail
{
inline std::uintptr_t GetThreadContextIp(CONTEXT const& context)
{
#if defined(HADESMEM_DETAIL_ARCH_X64)
  return context.Rip;
#elif defined(HADESMEM_DETAIL_ARCH_X86)
  return context.Eip;
#else
#error "[HadesMem] Unsupported architecture."
#endif
}

inline bool IsExecutingInRange(ThreadEntry const& thread_entry,
                               void const* beg,
                               void const* end)
{
  hadesmem::Thread const thread{thread_entry.GetId()};
  auto const context = GetThreadContext(thread, CONTEXT_CONTROL);
  auto const ip = reinterpret_cast<void const*>(
    hadesmem::detail::GetThreadContextIp(context));
  HADESMEM_DETAIL_ASSERT(ip);
  return ip >= beg && ip < end;
}
}
}

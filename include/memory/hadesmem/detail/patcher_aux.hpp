// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>

#include <windows.h>

#include <hadesmem/detail/thread_aux.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/thread_list.hpp>
#include <hadesmem/thread_helpers.hpp>

namespace hadesmem
{
namespace detail
{
// TODO: When detouring, instead of simply bailing in the case that
// VerifyPatchThreads fails, we should instead redirect the IP to the equivalent
// spot in our trampoline. Ditto for the reverse case when unhooking, we should
// redirect from our trampoline back to the target.
inline void VerifyPatchThreads(DWORD pid, void* target, std::size_t len)
{
  ThreadList threads{pid};
  for (auto const& thread_entry : threads)
  {
    if (thread_entry.GetId() == ::GetCurrentThreadId())
    {
      continue;
    }

    if (IsExecutingInRange(
          thread_entry, target, static_cast<std::uint8_t*>(target) + len))
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error{} << ErrorString{"Thread is currently executing patch target."});
    }
  }
}
}
}

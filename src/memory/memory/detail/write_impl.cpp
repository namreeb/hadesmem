// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/detail/write_impl.hpp"

#include <cassert>

#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/protect.hpp"
#include "hadesmem/detail/query_region.hpp"
#include "hadesmem/detail/protect_guard.hpp"

namespace hadesmem
{

namespace detail
{

void Write(Process const& process, PVOID address, LPCVOID data, 
  std::size_t len)
{
  assert(data != nullptr);
  assert(len != 0);

  ProtectGuard protect_guard(&process, address, ProtectGuardType::kWrite);
  
  SIZE_T bytes_written = 0;
  if (!::WriteProcessMemory(process.GetHandle(), address, data, 
    len, &bytes_written) || bytes_written != len)
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(Error() << 
      ErrorString("Could not write process memory.") << 
      ErrorCodeWinLast(last_error));
  }

  protect_guard.Restore();
}

}

}

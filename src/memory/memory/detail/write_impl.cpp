// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/detail/write_impl.hpp"

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/assert.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

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
  BOOST_ASSERT(data != nullptr);
  BOOST_ASSERT(len != 0);

  ProtectGuard protect_guard(process, address, ProtectGuardType::kWrite);
  
  SIZE_T bytes_written = 0;
  if (!::WriteProcessMemory(process.GetHandle(), address, data, 
    len, &bytes_written) || bytes_written != len)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Could not write process memory.") << 
      ErrorCodeWinLast(last_error));
  }

  protect_guard.Restore();
}

}

}

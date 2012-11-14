// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/detail/read_impl.hpp"

#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/protect.hpp"
#include "hadesmem/detail/query_region.hpp"
#include "hadesmem/detail/protect_guard.hpp"

namespace hadesmem
{

namespace detail
{

void Read(Process const& process, LPVOID address, LPVOID data, 
  std::size_t len)
{
  assert(data != nullptr);
  assert(len != 0);

  ProtectGuard protect_guard(&process, address, ProtectGuardType::kRead);
  
  SIZE_T bytes_read = 0;
  if (!::ReadProcessMemory(process.GetHandle(), address, data, len, 
    &bytes_read) || bytes_read != len)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Could not read process memory.") << 
      ErrorCodeWinLast(last_error));
  }

  protect_guard.Restore();
}

void ReadUnchecked(Process const& process, LPVOID address, LPVOID data, 
  std::size_t len)
{
  assert(data != nullptr);
  assert(len != 0);
  
  SIZE_T bytes_read = 0;
  if (!::ReadProcessMemory(process.GetHandle(), address, data, len, 
    &bytes_read) || bytes_read != len)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Could not read process memory.") << 
      ErrorCodeWinLast(last_error));
  }
}

}

}

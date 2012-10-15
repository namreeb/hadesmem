// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

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

void Read(Process const& process, LPVOID address, LPVOID out, 
  std::size_t out_size)
{
  BOOST_ASSERT(out != nullptr);
  BOOST_ASSERT(out_size != 0);

  ProtectGuard protect_guard(&process, address, ProtectGuardType::kRead);
  
  SIZE_T bytes_read = 0;
  if (!::ReadProcessMemory(process.GetHandle(), address, out, 
    out_size, &bytes_read) || bytes_read != out_size)
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(Error() << 
      ErrorString("Could not read process memory.") << 
      ErrorCodeWinLast(last_error));
  }

  protect_guard.Restore();
}

void ReadUnchecked(Process const& process, LPVOID address, LPVOID out, 
  std::size_t out_size)
{
  BOOST_ASSERT(out != nullptr);
  BOOST_ASSERT(out_size != 0);
  
  SIZE_T bytes_read = 0;
  if (!::ReadProcessMemory(process.GetHandle(), address, out, 
    out_size, &bytes_read) || bytes_read != out_size)
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(Error() << 
      ErrorString("Could not read process memory.") << 
      ErrorCodeWinLast(last_error));
  }
}

}

}

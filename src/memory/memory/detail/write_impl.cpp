// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/detail/write_impl.hpp"

#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/protect.hpp"
#include "hadesmem/detail/query_region.hpp"

namespace hadesmem
{

namespace detail
{

void Write(Process const& process, PVOID address, LPCVOID in, 
  std::size_t in_size)
{
  MEMORY_BASIC_INFORMATION const mbi = Query(process, address);
  
  if (IsGuard(mbi))
  {
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Attempt to write to guard page."));
  }
  
  bool const can_write = CanWrite(mbi);
  
  DWORD old_protect = 0;
  if (!can_write)
  {
    old_protect = Protect(process, address, PAGE_EXECUTE_READWRITE);
  }
  
  SIZE_T bytes_written = 0;
  if (!::WriteProcessMemory(process.GetHandle(), address, in, 
    in_size, &bytes_written) || bytes_written != in_size)
  {
    if (!can_write)
    {
      try
      {
        Protect(process, address, old_protect);
      }
      catch (std::exception const& /*e*/)
      { }
    }
    
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Could not write process memory.") << 
      ErrorCodeWinLast(last_error));
  }
  
  if (!can_write)
  {
    Protect(process, address, old_protect);
  }
}

}

}

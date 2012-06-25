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

namespace hadesmem
{

namespace detail
{

void Read(Process const& process, LPVOID address, LPVOID out, 
  std::size_t out_size)
{
  MEMORY_BASIC_INFORMATION const mbi = Query(process, address);
  
 if (IsGuard(mbi))
  {
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Attempt to read from guard page."));
  }
  
  bool const can_read = CanRead(mbi);

  DWORD old_protect = 0;
  if (!can_read)
  {
    old_protect = Protect(process, address, PAGE_EXECUTE_READWRITE);
  }
  
  SIZE_T bytes_read = 0;
  if (!::ReadProcessMemory(process.GetHandle(), address, out, 
    out_size, &bytes_read) || bytes_read != out_size)
  {
    if (!can_read)
    {
      try
      {
        Protect(process, address, old_protect);
      }
      catch (std::exception const& e)
      {
        (void)e;
        BOOST_ASSERT_MSG(false, boost::diagnostic_information(e).c_str());
      }
    }

    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Could not read process memory.") << 
      ErrorCodeWinLast(last_error));
  }
  
  if (!can_read)
  {
    Protect(process, address, old_protect);
  }
}

}

}

// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/detail/query_region.hpp"

#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"

namespace hadesmem
{

namespace detail
{

MEMORY_BASIC_INFORMATION Query(Process const& process, LPCVOID address)
{
  MEMORY_BASIC_INFORMATION mbi;
  ::ZeroMemory(&mbi, sizeof(mbi));
  if (::VirtualQueryEx(process.GetHandle(), address, &mbi, sizeof(mbi)) != 
    sizeof(mbi))
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("VirtualQueryEx failed.") << 
      ErrorCodeWinLast(last_error));
  }
  
  return mbi;
}

}

}

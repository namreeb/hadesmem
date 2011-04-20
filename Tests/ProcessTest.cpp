/*
This file is part of HadesMem.
Copyright (C) 2011 Joshua Boyce (a.k.a. RaptorFactor).
<http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

HadesMem is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

HadesMem is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with HadesMem.  If not, see <http://www.gnu.org/licenses/>.
*/

#define BOOST_TEST_MODULE ProcessTest
#pragma warning(push, 1)
#include <boost/test/unit_test.hpp>
#pragma warning(pop)

#include "HadesMemory/Memory.hpp"

BOOST_AUTO_TEST_CASE(BOOST_TEST_MODULE)
{
  // Todo: Test other constructors
  Hades::Memory::Process MyProcess(GetCurrentProcessId());
    
  BOOST_CHECK(MyProcess.GetHandle() != 0);
  BOOST_CHECK(MyProcess.GetID() != 0);
  BOOST_CHECK(!MyProcess.GetPath().empty());
  MyProcess.IsWoW64();
  
  for (Hades::Memory::ProcessIter Iter; *Iter; ++Iter)
  {
    Hades::Memory::Process const& CurProc = **Iter;
      
    BOOST_CHECK(CurProc.GetHandle() != 0);
    BOOST_CHECK(CurProc.GetID() != 0);
    try
    {
      // Wrap this block in EH because we're not running elevated so failure 
      // is a possibility
      BOOST_CHECK(!CurProc.GetPath().empty());
    }
    catch (Hades::HadesError const& /*e*/)
    { }
    CurProc.IsWoW64();
  }
}

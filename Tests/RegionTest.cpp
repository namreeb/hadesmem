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

#define BOOST_TEST_MODULE RegionTest
#pragma warning(push, 1)
#include <boost/test/unit_test.hpp>
#pragma warning(pop)

#include "HadesMemory/Memory.hpp"

// Todo: Proper checks/tests

BOOST_AUTO_TEST_CASE(BOOST_TEST_MODULE)
{
  Hades::Memory::MemoryMgr MyMemory(GetCurrentProcessId());
  
  for (Hades::Memory::RegionListIter i(MyMemory); *i; ++i)
  {
    Hades::Memory::Region Current = **i;
      
    Hades::Memory::Region Test(MyMemory, Current.GetBase());
      
    if (Test.GetState() != MEM_FREE)
    {
      BOOST_CHECK(Test.GetBase() != nullptr);
      BOOST_CHECK(Test.GetAllocBase() != nullptr);
      BOOST_CHECK(Test.GetAllocProtect() != 0);
      BOOST_CHECK(Test.GetType() != 0);
    }
    
    Test.GetProtect();
      
    BOOST_CHECK(Test.GetSize() != 0);
    BOOST_CHECK(Test.GetState() != 0);
    
    auto Path = boost::filesystem::unique_path();
      
    try
    {
      if (!MyMemory.IsGuard(Test.GetBase()) && MyMemory.CanRead(Test.GetBase()))
      {
        Test.Dump(Path);
      }
      
      boost::filesystem::remove(Path);
    }
    catch (...)
    {
      boost::filesystem::remove(Path);
        
      throw;
    }
  }
}

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

// Hades
#include "HadesMemory/Region.hpp"
#include "HadesMemory/MemoryMgr.hpp"

// Boost
#define BOOST_TEST_MODULE RegionTest
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

// Region component tests
BOOST_AUTO_TEST_CASE(BOOST_TEST_MODULE)
{
  // Create memory manager for self
  Hades::Memory::MemoryMgr MyMemory(GetCurrentProcessId());
  
  // Enumerate region list and run tests on all regions
  // Todo: Test region enumeration APIs before relying on them
  Hades::Memory::RegionList Regions(MyMemory);
  std::for_each(Regions.begin(), Regions.end(), 
    [&] (Hades::Memory::Region const& R)
    {
      // Test Region::Region
      Hades::Memory::Region Test(MyMemory, R.GetBase());
      
      // Test region member functions that must be non-zero on non-free blocks
      // Todo: Test Region::GetState before relying on its data
      if (Test.GetState() != MEM_FREE)
      {
        BOOST_CHECK(Test.GetBase() != nullptr);
        BOOST_CHECK(Test.GetAllocBase() != nullptr);
        BOOST_CHECK(Test.GetAllocProtect() != 0);
        BOOST_CHECK(Test.GetType() != 0);
      }
      
      // Ensure Section::GetProtect runs without exception (we have no 
      // predictable values for this API)
      Test.GetProtect();
        
      // Check predictable values for Section::GetSize and Section::GetState
      BOOST_CHECK(Test.GetSize() != 0);
      BOOST_CHECK(Test.GetState() != 0);
      
      // Generate path to write region dump to
      auto Path = boost::filesystem::unique_path();
      try
      {
        // Attempt to dump memory region
        // Todo: Ensure that Region::Dump succeeds for at least one region. 
        // Allocate new block with known data specifically for this purpose?
        if (!MyMemory.IsGuard(Test.GetBase()) && MyMemory.CanRead(Test.GetBase()))
        {
          Test.Dump(Path);
        }
        
        // Delete dump file
        boost::filesystem::remove(Path);
      }
      catch (...)
      {
        // Something went wrong. Delete dump file and rewthrow the error.
        boost::filesystem::remove(Path);
        throw;
      }
    });
}

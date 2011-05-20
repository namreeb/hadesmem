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
#include "HadesMemory/FindPattern.hpp"
#include "HadesMemory/MemoryMgr.hpp"

// Boost
#define BOOST_TEST_MODULE FindPatternTest
#include <boost/test/unit_test.hpp>

// FindPattern component tests
BOOST_AUTO_TEST_CASE(BOOST_TEST_MODULE)
{
  // Create memory manager for self
  Hades::Memory::MemoryMgr MyMemory(GetCurrentProcessId());
    
  // Create pattern scanner targetting self
  Hades::Memory::FindPattern MyFindPattern(MyMemory, GetModuleHandle(NULL));
  // Create pattern scanner targetting self (using default constructor this 
  // time)
  MyFindPattern = Hades::Memory::FindPattern(MyMemory);
  
  // Scan for predicatable byte masks and ensure that they were found and are 
  // different.
  auto pNop = MyFindPattern.Find(L"90", L"x");
  auto pZeros = MyFindPattern.Find(L"00 00 00", L"x?x");
  BOOST_CHECK(pNop != pZeros);
  BOOST_CHECK(pNop != nullptr);
  BOOST_CHECK(pZeros != nullptr);
  
  // Perform a full wildcard scan and ensure that both scans return the same 
  // pointer despite different data.
  auto pNopsAny = MyFindPattern.Find(L"90 90 90 90 90", L"?????");
  auto pInt3sAny = MyFindPattern.Find(L"CC CC CC CC CC", L"?????");
  BOOST_CHECK_EQUAL(pNopsAny, pInt3sAny);
  
  // Todo: Add tests for LoadFromXML, GetAddress, and operator[]
}

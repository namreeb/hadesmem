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

#define BOOST_TEST_MODULE FindPatternTest
#pragma warning(push, 1)
#include <boost/test/unit_test.hpp>
#pragma warning(pop)

#include "HadesMemory/Memory.hpp"

BOOST_AUTO_TEST_CASE(FindPatternTest)
{
  Hades::Memory::MemoryMgr MyMemory(GetCurrentProcessId());
    
  Hades::Memory::FindPattern MyFindPattern(MyMemory);
  MyFindPattern = Hades::Memory::FindPattern(MyMemory, GetModuleHandle(NULL));
    
  auto pNops = MyFindPattern.Find(L"90 90 90 90 90", L"xxxxx");
  auto pInt3s = MyFindPattern.Find(L"CC CC CC CC CC", L"xxxxx");
  BOOST_CHECK(pNops != pInt3s);
  
  auto pNopsAny = MyFindPattern.Find(L"90 90 90 90 90", L"?????");
  auto pInt3sAny = MyFindPattern.Find(L"CC CC CC CC CC", L"?????");
  BOOST_CHECK_EQUAL(pNopsAny, pInt3sAny);
  
  // Todo: Add tests for LoadFromXML, GetAddress, and operator[]
}

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

#define BOOST_TEST_MODULE PatcherTest
#pragma warning(push, 1)
#include <boost/test/unit_test.hpp>
#pragma warning(pop)

#include <algorithm>

#include "HadesMemory/Memory.hpp"

DWORD HookMe()
{
  std::string Foo("Foo");
  BOOST_CHECK_EQUAL(Foo, "Foo");
  return 0x1234;
}

std::shared_ptr<Hades::Memory::PatchDetour> pDetour1;

DWORD HookMe_Hook()
{
  BOOST_CHECK(pDetour1->GetTrampoline() != nullptr);
  auto pOrig = reinterpret_cast<DWORD (*)()>(pDetour1->GetTrampoline());
  pOrig();
  return 0x1337;
}

BOOST_AUTO_TEST_CASE(BOOST_TEST_MODULE)
{
  Hades::Memory::MemoryMgr MyMemory(GetCurrentProcessId());
    
  Hades::Memory::AllocAndFree TestMem1(MyMemory, 0x1000);
  std::vector<BYTE> Data1;
  Data1.push_back(0x00);
  Data1.push_back(0x11);
  Data1.push_back(0x22);
  Data1.push_back(0x33);
  Data1.push_back(0x44);
  Hades::Memory::PatchRaw Patch1(MyMemory, TestMem1.GetBase(), Data1);
  auto Orig1 = MyMemory.Read<std::vector<BYTE>>(TestMem1.GetBase(), 5);
  Patch1.Apply();
  auto Apply1 = MyMemory.Read<std::vector<BYTE>>(TestMem1.GetBase(), 5);
  Patch1.Remove();
  auto Remove1 = MyMemory.Read<std::vector<BYTE>>(TestMem1.GetBase(), 5);
  BOOST_CHECK(Orig1 == Remove1);
  BOOST_CHECK(Orig1 != Apply1);
  BOOST_CHECK(Data1 == Apply1);
  
  BOOST_CHECK_EQUAL(HookMe(), 0x1234);
  pDetour1.reset(new Hades::Memory::PatchDetour(MyMemory, &HookMe, 
    &HookMe_Hook));
  BOOST_CHECK_EQUAL(HookMe(), 0x1234);
  pDetour1->Apply();
  BOOST_CHECK_EQUAL(HookMe(), 0x1337);
  pDetour1->Remove();
  BOOST_CHECK_EQUAL(HookMe(), 0x1234);
  pDetour1->Apply();
  BOOST_CHECK_EQUAL(HookMe(), 0x1337);
  pDetour1->Remove();
  BOOST_CHECK_EQUAL(HookMe(), 0x1234);
}

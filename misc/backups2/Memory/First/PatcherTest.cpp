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
#include <HadesCommon/Config.hpp>
#include <HadesMemory/Patcher.hpp>
#include <HadesMemory/MemoryMgr.hpp>

// C++ Standard Library
#include <string>
#include <memory>

// Boost
#define BOOST_TEST_MODULE PatcherTest
#include <boost/test/unit_test.hpp>
#ifdef HADES_MSVC
#pragma warning(push, 1)
#endif
#include <boost/assign/std/vector.hpp>
#ifdef HADES_MSVC
#pragma warning(pop)
#endif
using namespace boost::assign;

// Function to be used as the detour target
DWORD HookMe()
{
  std::string const Foo("Foo");
  BOOST_CHECK_EQUAL(Foo, "Foo");
  return 0x1234;
}

// Detour manager
std::shared_ptr<Hades::Memory::PatchDetour> pDetour1;

// Function to be used as the detour replacement
DWORD HookMe_Hook()
{
  BOOST_CHECK(pDetour1->GetTrampoline() != nullptr);
  auto const pOrig = reinterpret_cast<DWORD (*)()>(reinterpret_cast<DWORD_PTR>(
    pDetour1->GetTrampoline()));
  BOOST_CHECK_EQUAL(pOrig(), static_cast<DWORD>(0x1234));
  return 0x1337;
}

// Patcher component tests
BOOST_AUTO_TEST_CASE(BOOST_TEST_MODULE)
{
  // Create memory manager for self
  Hades::Memory::MemoryMgr const MyMemory(GetCurrentProcessId());
  
  // Allocate memory block for use in byte patch tests
  Hades::Memory::AllocAndFree const TestMem1(MyMemory, 0x1000);
  // Create test data for use in byte patch tests
  std::vector<BYTE> Data1;
  Data1 += 0x00, 0x11, 0x22, 0x33, 0x44;
  BOOST_REQUIRE(Data1.size() == 5);
  // Create byte patcher
  Hades::Memory::PatchRaw Patch1(MyMemory, TestMem1.GetBase(), Data1);
  // Get original memory contents
  auto const Orig1 = MyMemory.Read<std::vector<BYTE>>(TestMem1.GetBase(), 5);
  // Apply patch
  Patch1.Apply();
  // Get patched memory contents
  auto const Apply1 = MyMemory.Read<std::vector<BYTE>>(TestMem1.GetBase(), 5);
  // Remove patch
  Patch1.Remove();
  // Get unpatched memory contents
  auto const Remove1 = MyMemory.Read<std::vector<BYTE>>(TestMem1.GetBase(), 5);
  // Ensure original and unpatched memory contents are the same
  BOOST_CHECK(Orig1 == Remove1);
  // Ensure original and patched memory contents are the same
  BOOST_CHECK(Orig1 != Apply1);
  // Ensure source data and patched memory contents are the same
  BOOST_CHECK(Data1 == Apply1);
  
  // Ensure detour target executes successfully
  BOOST_CHECK_EQUAL(HookMe(), static_cast<DWORD>(0x1234));
  // Create detour patcher
  DWORD_PTR const pHookMe = reinterpret_cast<DWORD_PTR>(&HookMe);
  DWORD_PTR const pHookMe_Hook = reinterpret_cast<DWORD_PTR>(&HookMe_Hook);
  pDetour1.reset(new Hades::Memory::PatchDetour(MyMemory, 
    reinterpret_cast<PVOID>(pHookMe), 
    reinterpret_cast<PVOID>(pHookMe_Hook)));
  // Ensure detour target executes successfully
  BOOST_CHECK_EQUAL(HookMe(), static_cast<DWORD>(0x1234));
  // Apply detour
  pDetour1->Apply();
  // Ensure detour replacement executes successfully
  BOOST_CHECK_EQUAL(HookMe(), static_cast<DWORD>(0x1337));
  // Remove detour
  pDetour1->Remove();
  // Ensure detour target executes successfully
  BOOST_CHECK_EQUAL(HookMe(), static_cast<DWORD>(0x1234));
  // Apply detour
  pDetour1->Apply();
  // Ensure detour replacement executes successfully
  BOOST_CHECK_EQUAL(HookMe(), static_cast<DWORD>(0x1337));
  // Remove detour
  pDetour1->Remove();
  // Ensure detour target executes successfully
  BOOST_CHECK_EQUAL(HookMe(), static_cast<DWORD>(0x1234));
}

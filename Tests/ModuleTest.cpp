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

// Haes
#include <HadesMemory/Module.hpp>
#include <HadesMemory/MemoryMgr.hpp>

// C++ Standard Library
#include <algorithm>

// Boost
#define BOOST_TEST_MODULE ModuleTest
#include <boost/test/unit_test.hpp>

// Module component tests
BOOST_AUTO_TEST_CASE(BOOST_TEST_MODULE)
{
  // Create memory manager for self
  Hades::Memory::MemoryMgr const MyMemory(GetCurrentProcessId());
    
  // Enumerate module list and run tests on all modules
  Hades::Memory::ModuleList Modules(MyMemory);
  std::for_each(Modules.begin(), Modules.end(), 
    [&] (Hades::Memory::Module const& M)
    {
      // Ensure module APIs execute without exception and return valid data
      BOOST_CHECK(M.GetBase() != 0);
      BOOST_CHECK(M.GetSize() != 0);
      BOOST_CHECK(!M.GetName().empty());
      BOOST_CHECK(!M.GetPath().empty());
      
      // Ensure GetREmoteModuleHandle works as expected
      // Note: The module name check could possibly fail if multiple modules 
      // with the same name but a different path are loaded in the process, 
      // but this is currently not the case with any of the testing binaries.
      BOOST_CHECK_EQUAL(M.GetBase(), Hades::Memory::GetRemoteModuleHandle(
        MyMemory, M.GetName().c_str()));
      BOOST_CHECK_EQUAL(M.GetBase(), Hades::Memory::GetRemoteModuleHandle(
        MyMemory, M.GetPath().c_str()));
      
      // Test module constructors
      Hades::Memory::Module const TestMod1(MyMemory, M.GetBase());
      Hades::Memory::Module const TestMod2(MyMemory, M.GetName());
      Hades::Memory::Module const TestMod3(MyMemory, M.GetPath().wstring());
      BOOST_CHECK_EQUAL(M.GetBase(), TestMod1.GetBase());
      BOOST_CHECK_EQUAL(M.GetBase(), TestMod2.GetBase());
      BOOST_CHECK_EQUAL(M.GetBase(), TestMod3.GetBase());
    });
}

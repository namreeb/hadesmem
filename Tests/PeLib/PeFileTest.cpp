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
#include <HadesMemory/MemoryMgr.hpp>
#include <HadesMemory/PeLib/PeFile.hpp>

// Boost
#define BOOST_TEST_MODULE PeFileTest
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(ConstructorsTest)
{
  // Create memory manager for self
  HadesMem::MemoryMgr MyMemory(GetCurrentProcessId());
    
  // Create PeFile
  HadesMem::PeFile MyPeFile(MyMemory, GetModuleHandle(NULL));
  
  // Test copying, assignement, and moving
  HadesMem::PeFile OtherPeFile(MyPeFile);
  BOOST_CHECK(MyPeFile == OtherPeFile);
  MyPeFile = OtherPeFile;
  BOOST_CHECK(MyPeFile == OtherPeFile);
  HadesMem::PeFile MovedPeFile(std::move(OtherPeFile));
  BOOST_CHECK(MovedPeFile == MyPeFile);
  HadesMem::PeFile NewTestPeFile(MyPeFile);
  MyPeFile = std::move(NewTestPeFile);
  BOOST_CHECK(MyPeFile == MovedPeFile);
}

BOOST_AUTO_TEST_CASE(DataTest)
{
  // Create memory manager for self
  HadesMem::MemoryMgr const MyMemory(GetCurrentProcessId());
  
  // Open self as a memory-based PeFile
  // Todo: Also test FileType_Data
  HadesMem::PeFile const MyPeFile(MyMemory, GetModuleHandle(NULL));
  
  // Check PeFile APIs for predictable values where possible, otherwise just 
  // ensure they run without exception
  MyPeFile.GetMemoryMgr();
  BOOST_CHECK_EQUAL(MyPeFile.GetMemoryMgr().GetProcessId(), 
    GetCurrentProcessId());
  BOOST_CHECK_EQUAL(MyPeFile.GetBase(), reinterpret_cast<PVOID>(
    GetModuleHandle(NULL)));
  BOOST_CHECK_EQUAL(MyPeFile.RvaToVa(0), static_cast<PVOID>(0));
  BOOST_CHECK_EQUAL(MyPeFile.GetType(), HadesMem::PeFile::
    FileType_Image);
}

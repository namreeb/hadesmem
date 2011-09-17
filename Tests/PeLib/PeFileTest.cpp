// Copyright Joshua Boyce 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

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

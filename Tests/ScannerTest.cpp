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
#include <HadesMemory/Scanner.hpp>
#include <HadesMemory/MemoryMgr.hpp>

// C++ Standard Library
#include <string>
#include <vector>
#include <algorithm>

// Boost
#define BOOST_TEST_MODULE ScannerTest
#include <boost/test/unit_test.hpp>

// Scanner component tests
BOOST_AUTO_TEST_CASE(BOOST_TEST_MODULE)
{
  // Create memory manager for self
  Hades::Memory::MemoryMgr const MyMemory(GetCurrentProcessId());
  
  // Allocate memory block to test scanner with
  // Todo: Tests which cross region boundaries
  Hades::Memory::AllocAndFree const MyMemBlock(MyMemory, 0x1000);
  PBYTE pMemBlock = static_cast<PBYTE>(MyMemBlock.GetBase());
  
  // Write test std::strings to memory block at known offset
  std::string const TestStringA("TestStringA");
  MyMemory.Write(pMemBlock + 0x000, TestStringA);
  MyMemory.Write(pMemBlock + 0x100, TestStringA);
      
  // Write test std::wstrings to memory block at known offset
  std::wstring const TestStringW(L"TestStringW");
  MyMemory.Write(pMemBlock + 0x200, TestStringW);
  MyMemory.Write(pMemBlock + 0x300, TestStringW);
    
  // Write test POD structures to memory block at known offset
  struct TestPODTypeT
  {
    int a;
    char* b;
    bool c;
  };
  TestPODTypeT const TestPODType = { 1337, reinterpret_cast<char*>(-1), true };
  MyMemory.Write(pMemBlock + 0x400, TestPODType);
  MyMemory.Write(pMemBlock + 0x500, TestPODType);
  
  // Write test vectors of POD structures to memory block at known offset
  TestPODTypeT const TestPODTypeOther = { 4321, reinterpret_cast<char*>(
    0x87654321), false };
  std::vector<TestPODTypeT> TestPODTypeVec;
  for (std::size_t i = 0; i != 10; ++i)
  {
    TestPODTypeVec.push_back(TestPODTypeOther);
  }
  MyMemory.Write(pMemBlock + 0x600, TestPODTypeVec);
  MyMemory.Write(pMemBlock + 0x700, TestPODTypeVec);
  
  // Create scanner targetting our memory block
  Hades::Memory::Scanner const MyScanner(MyMemory, MyMemBlock.GetBase(), 
    static_cast<PBYTE>(MyMemBlock.GetBase()) + MyMemBlock.GetSize());
  
  // Test Scanner::Find for T = std::string
  PVOID const pTestStringA = MyScanner.Find(TestStringA);
  BOOST_CHECK(pTestStringA == pMemBlock + 0x000);
  // Test Scanner::FindAll for T = std::string
  std::vector<PVOID> const pTestStringAVec = MyScanner.FindAll(TestStringA);
  BOOST_CHECK_EQUAL(pTestStringAVec.size(), static_cast<std::size_t>(2));
  std::for_each(pTestStringAVec.cbegin(), pTestStringAVec.cend(), 
    [=] (PVOID p)
    {
      BOOST_CHECK(p == pMemBlock + 0x000 || p == pMemBlock + 0x100);
    });
  
  // Test Scanner::Find for T = std::wstring
  PVOID const pTestStringW = MyScanner.Find(TestStringW);
  BOOST_CHECK(pTestStringW == pMemBlock + 0x200);
  // Test Scanner::FindAll for T = std::wstring
  std::vector<PVOID> const pTestStringWVec = MyScanner.FindAll(TestStringW);
  BOOST_CHECK_EQUAL(pTestStringWVec.size(), static_cast<std::size_t>(2));
  std::for_each(pTestStringWVec.cbegin(), pTestStringWVec.cend(), 
    [=] (PVOID p)
    {
      BOOST_CHECK(p == pMemBlock + 0x200 || p == pMemBlock + 0x300);
    });
  
  // Test Scanner::Find for T = POD type
  PVOID const pTestPODType = MyScanner.Find(TestPODType);
  BOOST_CHECK(pTestPODType == pMemBlock + 0x400);
  // Test Scanner::FindAll for T = POD type
  std::vector<PVOID> const TestPODTypeAddrList = MyScanner.FindAll(
    TestPODType);
  BOOST_CHECK_EQUAL(TestPODTypeAddrList.size(), static_cast<std::size_t>(2));
  std::for_each(TestPODTypeAddrList.cbegin(), TestPODTypeAddrList.cend(), 
    [=] (PVOID p)
    {
      BOOST_CHECK(p == pMemBlock + 0x400 || p == pMemBlock + 0x500);
    });
  
  // Test Scanner::Find for T = std::vector<U> where U = POD type
  PVOID const pTestPODTypeVec = MyScanner.Find(TestPODTypeVec);
  BOOST_CHECK(pTestPODTypeVec == pMemBlock + 0x600);
  // Test Scanner::FindAll for T = std::vector<U> where U = POD type
  std::vector<PVOID> const TestPODTypeVecAddrList = MyScanner.FindAll(
    TestPODTypeVec);
  BOOST_CHECK_EQUAL(TestPODTypeVecAddrList.size(), static_cast<std::size_t>(
    2));
  std::for_each(TestPODTypeVecAddrList.cbegin(), 
    TestPODTypeVecAddrList.cend(), 
    [=] (PVOID p)
    {
      BOOST_CHECK(p == pMemBlock + 0x600 || p == pMemBlock + 0x700);
    });
}

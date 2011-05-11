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

#define BOOST_TEST_MODULE ScannerTest
#include <boost/test/unit_test.hpp>

#include <algorithm>

#include "HadesMemory/Scanner.hpp"
#include "HadesMemory/MemoryMgr.hpp"

BOOST_AUTO_TEST_CASE(BOOST_TEST_MODULE)
{
  Hades::Memory::MemoryMgr MyMemory(GetCurrentProcessId());
  
  Hades::Memory::AllocAndFree MyMemBlock(MyMemory, 0x1000);
  PBYTE pMemBlock = static_cast<PBYTE>(MyMemBlock.GetBase());
  
  std::string const TestStringA("TestStringA");
  MyMemory.Write(pMemBlock + 0x000, TestStringA);
  MyMemory.Write(pMemBlock + 0x100, TestStringA);
      
  std::wstring const TestStringW(L"TestStringW");
  MyMemory.Write(pMemBlock + 0x200, TestStringW);
  MyMemory.Write(pMemBlock + 0x300, TestStringW);
    
  struct TestPODTypeT
  {
    int a;
    char* b;
    bool c;
  };
  TestPODTypeT TestPODType = { 1337, reinterpret_cast<char*>(-1), true };
  MyMemory.Write(pMemBlock + 0x400, TestPODType);
  MyMemory.Write(pMemBlock + 0x500, TestPODType);
  
  TestPODTypeT TestPODTypeOther = { 4321, reinterpret_cast<char*>(0x87654321), 
    false };
  std::vector<TestPODTypeT> TestPODTypeVec;
  for (std::size_t i = 0; i != 10; ++i)
  {
    TestPODTypeVec.push_back(TestPODTypeOther);
  }
  MyMemory.Write(pMemBlock + 0x600, TestPODTypeVec);
  MyMemory.Write(pMemBlock + 0x700, TestPODTypeVec);
  
  Hades::Memory::Scanner MyScanner(MyMemory, MyMemBlock.GetBase(), 
    static_cast<PBYTE>(MyMemBlock.GetBase()) + MyMemBlock.GetSize());
    
  PVOID pTestStringA = MyScanner.Find(TestStringA);
  BOOST_CHECK(pTestStringA == pMemBlock + 0x000);
  std::vector<PVOID> pTestStringAVec = MyScanner.FindAll(TestStringA);
  BOOST_CHECK_EQUAL(pTestStringAVec.size(), static_cast<std::size_t>(2));
  std::for_each(pTestStringAVec.cbegin(), pTestStringAVec.cend(), 
    [=] (PVOID p)
    {
      BOOST_CHECK(p == pMemBlock + 0x000 || p == pMemBlock + 0x100);
    });
  
  PVOID pTestStringW = MyScanner.Find(TestStringW);
  BOOST_CHECK(pTestStringW == pMemBlock + 0x200);
  std::vector<PVOID> pTestStringWVec = MyScanner.FindAll(TestStringW);
  BOOST_CHECK_EQUAL(pTestStringWVec.size(), static_cast<std::size_t>(2));
  std::for_each(pTestStringWVec.cbegin(), pTestStringWVec.cend(), 
    [=] (PVOID p)
    {
      BOOST_CHECK(p == pMemBlock + 0x200 || p == pMemBlock + 0x300);
    });
  
  PVOID pTestPODType = MyScanner.Find(TestPODType);
  BOOST_CHECK(pTestPODType == pMemBlock + 0x400);
  std::vector<PVOID> TestPODTypeAddrList = MyScanner.FindAll(TestPODType);
  BOOST_CHECK_EQUAL(TestPODTypeAddrList.size(), static_cast<std::size_t>(2));
  std::for_each(TestPODTypeAddrList.cbegin(), TestPODTypeAddrList.cend(), 
    [=] (PVOID p)
    {
      BOOST_CHECK(p == pMemBlock + 0x400 || p == pMemBlock + 0x500);
    });
  
  PVOID pTestPODTypeVec = MyScanner.Find(TestPODTypeVec);
  BOOST_CHECK(pTestPODTypeVec == pMemBlock + 0x600);
  std::vector<PVOID> TestPODTypeVecAddrList = MyScanner.FindAll(TestPODTypeVec);
  BOOST_CHECK_EQUAL(TestPODTypeVecAddrList.size(), static_cast<std::size_t>(2));
  std::for_each(TestPODTypeVecAddrList.cbegin(), 
    TestPODTypeVecAddrList.cend(), 
    [=] (PVOID p)
    {
      BOOST_CHECK(p == pMemBlock + 0x600 || p == pMemBlock + 0x700);
    });
}

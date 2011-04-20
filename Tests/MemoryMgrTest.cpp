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

#define BOOST_TEST_MODULE MemoryMgrTest
#pragma warning(push, 1)
#include <boost/test/unit_test.hpp>
#pragma warning(pop)

#include <algorithm>

#include "HadesMemory/Memory.hpp"

DWORD_PTR TestCall(PVOID a, PVOID b, PVOID c, PVOID d)
{
  BOOST_CHECK_EQUAL(a, static_cast<PVOID>(nullptr));
  BOOST_CHECK_EQUAL(b, reinterpret_cast<PVOID>(-1));
  BOOST_CHECK_EQUAL(c, reinterpret_cast<PVOID>(0x11223344));
  BOOST_CHECK_EQUAL(d, reinterpret_cast<PVOID>(0xAABBCCDD));
  
  SetLastError(5678);
  return 1234;
}

struct TestPODType
{
  int a;
  char* b;
  wchar_t c;
  long long d;
};

BOOST_AUTO_TEST_CASE(BOOST_TEST_MODULE)
{
  Hades::Memory::MemoryMgr MyMemory(GetCurrentProcessId());
  
  std::vector<PVOID> TestCallArgs;
  TestCallArgs.push_back(nullptr);
  TestCallArgs.push_back(reinterpret_cast<PVOID>(-1));
  TestCallArgs.push_back(reinterpret_cast<PVOID>(0x11223344));
  TestCallArgs.push_back(reinterpret_cast<PVOID>(0xAABBCCDD));
  std::pair<DWORD_PTR, DWORD> CallRet = MyMemory.Call(&TestCall, TestCallArgs);
  BOOST_CHECK_EQUAL(CallRet.first, 1234);
  BOOST_CHECK_EQUAL(CallRet.second, 5678);
  
  TestPODType MyTestPODType = { 1, 0, L'a', 1234567812345678 };
  auto MyNewTestPODType = MyMemory.Read<TestPODType>(&MyTestPODType);
  BOOST_CHECK_EQUAL(MyNewTestPODType.a, MyTestPODType.a);
  BOOST_CHECK_EQUAL(MyNewTestPODType.b, MyTestPODType.b);
  BOOST_CHECK_EQUAL(MyNewTestPODType.c, MyTestPODType.c);
  BOOST_CHECK_EQUAL(MyNewTestPODType.d, MyTestPODType.d);
  TestPODType MyTestPODType2 = { -1, 0, L'x', 9876543210 };
  MyMemory.Write(&MyTestPODType, MyTestPODType2);
  BOOST_CHECK_EQUAL(MyTestPODType2.a, MyTestPODType.a);
  BOOST_CHECK_EQUAL(MyTestPODType2.b, MyTestPODType.b);
  BOOST_CHECK_EQUAL(MyTestPODType2.c, MyTestPODType.c);
  BOOST_CHECK_EQUAL(MyTestPODType2.d, MyTestPODType.d);
  
  char const* pTestStringA = "Narrow test string.";
  char* pTestStringAReal = const_cast<char*>(pTestStringA);
  auto NewTestStringA = MyMemory.Read<std::string>(pTestStringAReal);
  BOOST_CHECK_EQUAL(NewTestStringA, pTestStringA);
  auto TestStringAStr = std::string(pTestStringA);
  auto TestStringARev = std::string(TestStringAStr.rbegin(), 
    TestStringAStr.rend());
  MyMemory.Write(pTestStringAReal, TestStringARev);
  auto NewTestStringARev = MyMemory.Read<std::string>(pTestStringAReal);
  BOOST_CHECK_EQUAL(NewTestStringARev, TestStringARev);
  
  wchar_t const* pTestStringW = L"Wide test string.";
  wchar_t* pTestStringWReal = const_cast<wchar_t*>(pTestStringW);
  auto NewTestStringW = MyMemory.Read<std::wstring>(pTestStringWReal);
  // Note: BOOST_CHECK_EQUAL does not support wide strings it seems
  BOOST_CHECK(NewTestStringW == pTestStringW);
  auto TestStringWStr = std::wstring(pTestStringW);
  auto TestStringWRev = std::wstring(TestStringWStr.rbegin(), 
    TestStringWStr.rend());
  MyMemory.Write(pTestStringWReal, TestStringWRev);
  auto NewTestStringWRev = MyMemory.Read<std::wstring>(pTestStringWReal);
  // Note: BOOST_CHECK_EQUAL does not support wide strings it seems
  BOOST_CHECK(NewTestStringWRev == TestStringWRev);
  
  BOOST_CHECK_EQUAL(MyMemory.CanRead(GetModuleHandle(NULL)), true);
  int ReadTestStack = 0;
  BOOST_CHECK_EQUAL(MyMemory.CanRead(&ReadTestStack), true);
  
  BOOST_CHECK_EQUAL(MyMemory.CanWrite(GetModuleHandle(NULL)), false);
  int WriteTestStack = 0;
  BOOST_CHECK_EQUAL(MyMemory.CanWrite(&WriteTestStack), true);
  
  
  BOOST_CHECK_EQUAL(MyMemory.IsGuard(GetModuleHandle(NULL)), false);
  int GuardTestStack = 0;
  BOOST_CHECK_EQUAL(MyMemory.IsGuard(&GuardTestStack), false);
  
  auto pNewMemBlock = MyMemory.Alloc(0x1000);
  BOOST_CHECK(pNewMemBlock != nullptr);
  MyMemory.Free(pNewMemBlock);
  
  BOOST_CHECK_EQUAL(MyMemory.GetProcessID(), GetCurrentProcessId());
  
  BOOST_CHECK_EQUAL(MyMemory.GetProcessHandle(), GetCurrentProcess());
  
  HMODULE Kernel32Mod = GetModuleHandle(L"kernel32.dll");
  BOOST_REQUIRE(Kernel32Mod != nullptr);
  
  FARPROC pGetCurrentProcessId = GetProcAddress(Kernel32Mod, 
    "GetCurrentProcessId");
  BOOST_REQUIRE(pGetCurrentProcessId != nullptr);
  
  BOOST_CHECK_EQUAL(MyMemory.GetRemoteProcAddress(Kernel32Mod, 
    "kernel32.dll", "GetCurrentProcessId"), pGetCurrentProcessId);
    
  MyMemory.FlushCache(&TestCall, 10);
    
  MyMemory.IsWoW64();
    
  // Todo: Test GetRemoteProcAddress with ordinals
}

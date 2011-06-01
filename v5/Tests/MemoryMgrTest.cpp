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

// Boost
#define BOOST_TEST_MODULE MemoryMgrTest
#include <boost/test/unit_test.hpp>

// Test function to be called by MemoryMgr::Call
DWORD_PTR TestCall(PVOID const a, PVOID const b, PVOID const c, PVOID const d)
{
  BOOST_CHECK_EQUAL(a, static_cast<PVOID>(nullptr));
  BOOST_CHECK_EQUAL(b, reinterpret_cast<PVOID>(-1));
  BOOST_CHECK_EQUAL(c, reinterpret_cast<PVOID>(0x11223344));
  BOOST_CHECK_EQUAL(d, reinterpret_cast<PVOID>(0xAABBCCDD));
  
  SetLastError(5678);
  return 1234;
}

// MemoryMgr component tests
BOOST_AUTO_TEST_CASE(BOOST_TEST_MODULE)
{
  // Create memory manager for self
  // Todo: Test other constructors
  Hades::Memory::MemoryMgr const MyMemory(GetCurrentProcessId());
  
  // Call test function and ensure returned data is valid
  std::vector<PVOID> TestCallArgs;
  TestCallArgs.push_back(nullptr);
  TestCallArgs.push_back(reinterpret_cast<PVOID>(-1));
  TestCallArgs.push_back(reinterpret_cast<PVOID>(0x11223344));
  TestCallArgs.push_back(reinterpret_cast<PVOID>(0xAABBCCDD));
  std::pair<DWORD_PTR, DWORD> const CallRet = MyMemory.Call(
    reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(&TestCall)), 
    TestCallArgs);
  BOOST_CHECK_EQUAL(CallRet.first, static_cast<DWORD_PTR>(1234));
  BOOST_CHECK_EQUAL(CallRet.second, static_cast<DWORD>(5678));
  
  // Test POD type for testing MemoryMgr::Read/MemoryMgr::Write
  struct TestPODType
  {
    int a;
    char* b;
    wchar_t c;
    long long d;
  };

  // Test MemoryMgr::Read and MemoryMgr::Write for POD types
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
  
  // Test MemoryMgr::Read and MemoryMgr::Write for std::string
  char const* const pTestStringA = "Narrow test string.";
  char* const pTestStringAReal = const_cast<char*>(pTestStringA);
  auto const NewTestStringA = MyMemory.Read<std::string>(pTestStringAReal);
  BOOST_CHECK_EQUAL(NewTestStringA, pTestStringA);
  auto const TestStringAStr = std::string(pTestStringA);
  auto const TestStringARev = std::string(TestStringAStr.rbegin(), 
    TestStringAStr.rend());
  MyMemory.Write(pTestStringAReal, TestStringARev);
  auto const NewTestStringARev = MyMemory.Read<std::string>(pTestStringAReal);
  BOOST_CHECK_EQUAL(NewTestStringARev, TestStringARev);
  
  // Test MemoryMgr::Read and MemoryMgr::Write for std::wstring
  wchar_t const* const pTestStringW = L"Wide test string.";
  wchar_t* const pTestStringWReal = const_cast<wchar_t*>(pTestStringW);
  auto const NewTestStringW = MyMemory.Read<std::wstring>(pTestStringWReal);
  // Note: BOOST_CHECK_EQUAL does not support wide strings it seems
  BOOST_CHECK(NewTestStringW == pTestStringW);
  auto const TestStringWStr = std::wstring(pTestStringW);
  auto const TestStringWRev = std::wstring(TestStringWStr.rbegin(), 
    TestStringWStr.rend());
  MyMemory.Write(pTestStringWReal, TestStringWRev);
  auto const NewTestStringWRev = MyMemory.Read<std::wstring>(pTestStringWReal);
  // Note: BOOST_CHECK_EQUAL does not support wide strings it seems
  BOOST_CHECK(NewTestStringWRev == TestStringWRev);
  
  // Test MemoryMgr::CanRead
  BOOST_CHECK_EQUAL(MyMemory.CanRead(GetModuleHandle(NULL)), true);
  int const ReadTestStack = 0;
  BOOST_CHECK_EQUAL(MyMemory.CanRead(&ReadTestStack), true);
  
  // Test MemoryMgr::CanWrite
  BOOST_CHECK_EQUAL(MyMemory.CanWrite(GetModuleHandle(NULL)), false);
  int const WriteTestStack = 0;
  BOOST_CHECK_EQUAL(MyMemory.CanWrite(&WriteTestStack), true);
  
  // Test MemoryMgr::IsGuard
  BOOST_CHECK_EQUAL(MyMemory.IsGuard(GetModuleHandle(NULL)), false);
  int const GuardTestStack = 0;
  BOOST_CHECK_EQUAL(MyMemory.IsGuard(&GuardTestStack), false);
  
  // Test MemoryMgr::Alloc and MemoryMgr::Free
  auto pNewMemBlock = MyMemory.Alloc(0x1000);
  BOOST_CHECK(pNewMemBlock != nullptr);
  MyMemory.Free(pNewMemBlock);
  
  // Test AllocAndFree
  {
    Hades::Memory::AllocAndFree const MyAllocTest(MyMemory, 0x1000);
  }
  
  // Test MemoryMgr::GetProcessID
  BOOST_CHECK_EQUAL(MyMemory.GetProcessID(), GetCurrentProcessId());
  
  // Test MemoryMgr::GetProcessHandle
  BOOST_CHECK_EQUAL(MyMemory.GetProcessHandle(), GetCurrentProcess());
  
  // Ensure we can find a valid Kernel32 instance
  HMODULE const Kernel32Mod = GetModuleHandle(L"kernel32.dll");
  BOOST_REQUIRE(Kernel32Mod != nullptr);
  
  // Ensure we can find Kernel32.dll!GetCurrentProcessId
  FARPROC const pGetCurrentProcessId = GetProcAddress(Kernel32Mod, 
    "GetCurrentProcessId");
  BOOST_REQUIRE(pGetCurrentProcessId != nullptr);
  
  // 'Remotely' call Kernel32.dll!GetCurrentProcessId and ensure its result 
  // matches the local results.
  // Todo: Test GetRemoteProcAddress with ordinals
  BOOST_CHECK_EQUAL(MyMemory.GetRemoteProcAddress(Kernel32Mod, 
    "kernel32.dll", "GetCurrentProcessId"), pGetCurrentProcessId);
  
  // Test MemoryMgr::FlushCache
  MyMemory.FlushCache(reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
    &TestCall)), 10);
  
  // Todo: Verify MemoryMgr::IsWoW64 return value
  MyMemory.IsWoW64();
}

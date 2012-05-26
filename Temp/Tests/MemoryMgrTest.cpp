// Copyright Joshua Boyce 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

// TODO:
// Test CreateProcess
// Test GetSeDebugPrivilege

// Hades
#include <HadesMemory/MemoryMgr.hpp>

// C++ Standard Library
#include <cmath>
#include <array>

// Boost
#define BOOST_TEST_MODULE MemoryMgrTest
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(ConstructorsTest)
{
  // Create memory manager for self
  HadesMem::MemoryMgr MyMemory(GetCurrentProcessId());
  HadesMem::MemoryMgr OtherMemory(MyMemory);
  BOOST_CHECK(MyMemory == OtherMemory);
  MyMemory = OtherMemory;
  BOOST_CHECK_THROW(HadesMem::MemoryMgr InvalidMemory(
    static_cast<DWORD>(-1)), HadesMem::HadesMemError);
  HadesMem::MemoryMgr MovedMemory(std::move(OtherMemory));
  MyMemory = std::move(MovedMemory);
  BOOST_CHECK_EQUAL(MyMemory.GetProcessId(), GetCurrentProcessId());
}

// Test function to be called by MemoryMgr::Call
DWORD_PTR TestCall(PVOID const a, PVOID const b, PVOID const c, PVOID const d, 
  PVOID const e, PVOID const f)
{
  BOOST_CHECK_EQUAL(a, static_cast<PVOID>(nullptr));
  BOOST_CHECK_EQUAL(b, reinterpret_cast<PVOID>(-1));
  BOOST_CHECK_EQUAL(c, reinterpret_cast<PVOID>(0x11223344));
  BOOST_CHECK_EQUAL(d, reinterpret_cast<PVOID>(0xAABBCCDD));
  BOOST_CHECK_EQUAL(e, reinterpret_cast<PVOID>(0x55667788));
  BOOST_CHECK_EQUAL(f, reinterpret_cast<PVOID>(0x99999999));
  
  SetLastError(5678);
  return 1234;
}

// Test function to be called by MemoryMgr::Call
DWORD64 TestCall64Ret()
{
  return 0x123456787654321LL;
}

#if defined(_M_AMD64) 
#elif defined(_M_IX86) 
// Test function to be called by MemoryMgr::Call
DWORD_PTR __fastcall TestFastCall(PVOID const a, PVOID const b, PVOID const c, 
  PVOID const d, PVOID const e, PVOID const f)
{
  BOOST_CHECK_EQUAL(a, static_cast<PVOID>(nullptr));
  BOOST_CHECK_EQUAL(b, reinterpret_cast<PVOID>(-1));
  BOOST_CHECK_EQUAL(c, reinterpret_cast<PVOID>(0x11223344));
  BOOST_CHECK_EQUAL(d, reinterpret_cast<PVOID>(0xAABBCCDD));
  BOOST_CHECK_EQUAL(e, reinterpret_cast<PVOID>(0x55667788));
  BOOST_CHECK_EQUAL(f, reinterpret_cast<PVOID>(0x99999999));
  
  SetLastError(5678);
  return 1234;
}

// Test function to be called by MemoryMgr::Call
DWORD_PTR __stdcall TestStdCall(PVOID const a, PVOID const b, PVOID const c, 
  PVOID const d, PVOID const e, PVOID const f)
{
  BOOST_CHECK_EQUAL(a, static_cast<PVOID>(nullptr));
  BOOST_CHECK_EQUAL(b, reinterpret_cast<PVOID>(-1));
  BOOST_CHECK_EQUAL(c, reinterpret_cast<PVOID>(0x11223344));
  BOOST_CHECK_EQUAL(d, reinterpret_cast<PVOID>(0xAABBCCDD));
  BOOST_CHECK_EQUAL(e, reinterpret_cast<PVOID>(0x55667788));
  BOOST_CHECK_EQUAL(f, reinterpret_cast<PVOID>(0x99999999));
  
  SetLastError(5678);
  return 1234;
}
#else 
#error "[HadesMem] Unsupported architecture."
#endif

BOOST_AUTO_TEST_CASE(CallTest)
{
  // Create memory manager for self
  HadesMem::MemoryMgr MyMemory(GetCurrentProcessId());

  // Call test function and ensure returned data is valid
  std::vector<PVOID> TestCallArgs;
  TestCallArgs.push_back(nullptr);
  TestCallArgs.push_back(reinterpret_cast<PVOID>(-1));
  TestCallArgs.push_back(reinterpret_cast<PVOID>(0x11223344));
  TestCallArgs.push_back(reinterpret_cast<PVOID>(0xAABBCCDD));
  TestCallArgs.push_back(reinterpret_cast<PVOID>(0x55667788));
  TestCallArgs.push_back(reinterpret_cast<PVOID>(0x99999999));
  HadesMem::MemoryMgr::RemoteFunctionRet const CallRet = MyMemory.Call(
    reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(&TestCall)), 
    HadesMem::MemoryMgr::CallConv_Default, TestCallArgs);
  BOOST_CHECK_EQUAL(CallRet.GetReturnValue(), static_cast<DWORD_PTR>(1234));
  BOOST_CHECK_EQUAL(CallRet.GetLastError(), static_cast<DWORD>(5678));
  
  // Test __fastcall and __stdcall under x86
#if defined(_M_AMD64) 
#elif defined(_M_IX86) 
  HadesMem::MemoryMgr::RemoteFunctionRet const CallRetFast = 
    MyMemory.Call(reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
    &TestFastCall)), HadesMem::MemoryMgr::CallConv_FASTCALL, 
    TestCallArgs);
  BOOST_CHECK_EQUAL(CallRetFast.GetReturnValue(), static_cast<DWORD_PTR>(
    1234));
  BOOST_CHECK_EQUAL(CallRetFast.GetLastError(), static_cast<DWORD>(5678));
  
  HadesMem::MemoryMgr::RemoteFunctionRet const CallRetStd = 
    MyMemory.Call(reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
    &TestStdCall)), HadesMem::MemoryMgr::CallConv_STDCALL, TestCallArgs);
  BOOST_CHECK_EQUAL(CallRetStd.GetReturnValue(), static_cast<DWORD_PTR>(
    1234));
  BOOST_CHECK_EQUAL(CallRetStd.GetLastError(), static_cast<DWORD>(5678));
#else 
#error "[HadesMem] Unsupported architecture."
#endif

  // Test 64-bit return values in MemoryMgr::Call
  std::vector<PVOID> TestCall64Args;
  HadesMem::MemoryMgr::RemoteFunctionRet const CallRet64 = MyMemory.Call(
    reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(&TestCall64Ret)), 
    HadesMem::MemoryMgr::CallConv_Default, TestCall64Args);
  BOOST_CHECK_EQUAL(CallRet64.GetReturnValue64(), static_cast<DWORD64>(
    0x123456787654321LL));
}

BOOST_AUTO_TEST_CASE(ReadWriteTest)
{
  // Create memory manager for self
  HadesMem::MemoryMgr MyMemory(GetCurrentProcessId());

  // Test POD type for testing MemoryMgr::Read/MemoryMgr::Write
  struct TestPODType
  {
    int a;
    char* b;
    wchar_t c;
    long long d;
  };

  // Test MemoryMgr::Read and MemoryMgr::Write
  TestPODType MyTestPODType = { 1, 0, L'a', 1234567812345678 };
  auto MyNewTestPODType = MyMemory.Read<TestPODType>(&MyTestPODType);
  BOOST_CHECK_EQUAL(std::memcmp(&MyTestPODType, &MyNewTestPODType, 
    sizeof(TestPODType)), 0);
  TestPODType MyTestPODType2 = { -1, 0, L'x', 9876543210 };
  MyMemory.Write(&MyTestPODType, MyTestPODType2);
  BOOST_CHECK_EQUAL(std::memcmp(&MyTestPODType, &MyTestPODType2, 
    sizeof(TestPODType)), 0);
  
  // Test MemoryMgr::ReadString and MemoryMgr::WriteString for std::string
  char const* const pTestStringA = "Narrow test string.";
  char* const pTestStringAReal = const_cast<char*>(pTestStringA);
  auto const NewTestStringA = MyMemory.ReadString<std::string>(
    pTestStringAReal);
  BOOST_CHECK_EQUAL(NewTestStringA, pTestStringA);
  auto const TestStringAStr = std::string(pTestStringA);
  auto const TestStringARev = std::string(TestStringAStr.rbegin(), 
    TestStringAStr.rend());
  MyMemory.WriteString(pTestStringAReal, TestStringARev);
  auto const NewTestStringARev = MyMemory.ReadString<std::string>(
    pTestStringAReal);
  BOOST_CHECK_EQUAL_COLLECTIONS(NewTestStringARev.cbegin(), 
    NewTestStringARev.cend(), TestStringARev.cbegin(), TestStringARev.cend());
  
  // Test MemoryMgr::ReadString and MemoryMgr::WriteString for std::wstring
  wchar_t const* const pTestStringW = L"Wide test string.";
  wchar_t* const pTestStringWReal = const_cast<wchar_t*>(pTestStringW);
  auto const NewTestStringW = MyMemory.ReadString<std::wstring>(
    pTestStringWReal);
  // Note: BOOST_CHECK_EQUAL does not support wide strings it seems
  BOOST_CHECK(NewTestStringW == pTestStringW);
  auto const TestStringWStr = std::wstring(pTestStringW);
  auto const TestStringWRev = std::wstring(TestStringWStr.rbegin(), 
    TestStringWStr.rend());
  MyMemory.WriteString(pTestStringWReal, TestStringWRev);
  auto const NewTestStringWRev = MyMemory.ReadString<std::wstring>(
    pTestStringWReal);
  // Note: BOOST_CHECK_EQUAL does not support wide strings it seems
  BOOST_CHECK_EQUAL_COLLECTIONS(NewTestStringWRev.cbegin(), 
    NewTestStringWRev.cend(), TestStringWRev.cbegin(), TestStringWRev.cend());
  
  // Test MemoryMgr::ReadList and MemoryMgr::WriteList
  std::array<int, 10> IntList = {{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }};
  std::vector<int> IntListRead = MyMemory.ReadList<std::vector<int>>(&IntList, 
    10);
  BOOST_CHECK_EQUAL_COLLECTIONS(IntList.cbegin(), IntList.cend(), 
    IntListRead.cbegin(), IntListRead.cend());
  std::vector<int> IntListRev(IntListRead.crbegin(), IntListRead.crend());
  MyMemory.WriteList(&IntList, IntListRev);
  BOOST_CHECK_EQUAL_COLLECTIONS(IntList.cbegin(), IntList.cend(), 
    IntListRev.cbegin(), IntListRev.cend());
}

BOOST_AUTO_TEST_CASE(RegionTest)
{
  // Create memory manager for self
  HadesMem::MemoryMgr MyMemory(GetCurrentProcessId());

  // Test MemoryMgr::CanRead
  BOOST_CHECK_EQUAL(MyMemory.CanRead(GetModuleHandle(NULL)), true);
  int const ReadTestStack = 0;
  BOOST_CHECK_EQUAL(MyMemory.CanRead(&ReadTestStack), true);
  
  // Test MemoryMgr::CanWrite
  BOOST_CHECK_EQUAL(MyMemory.CanWrite(GetModuleHandle(NULL)), false);
  int const WriteTestStack = 0;
  BOOST_CHECK_EQUAL(MyMemory.CanWrite(&WriteTestStack), true);
  
  // Test MemoryMgr::CanExecute
  BOOST_CHECK_EQUAL(MyMemory.CanExecute(reinterpret_cast<PVOID>(
    reinterpret_cast<DWORD_PTR>(&TestCall))), true);
  int const ExecuteTestStack = 0;
  BOOST_CHECK_EQUAL(MyMemory.CanExecute(&ExecuteTestStack), false);
  
  // Test MemoryMgr::IsGuard
  BOOST_CHECK_EQUAL(MyMemory.IsGuard(GetModuleHandle(NULL)), false);
  int const GuardTestStack = 0;
  BOOST_CHECK_EQUAL(MyMemory.IsGuard(&GuardTestStack), false);
  
  // Test MemoryMgr::Alloc, MemoryMgr::ProtectRegion, and MemoryMgr::Free
  auto const pNewMemBlock = MyMemory.Alloc(0x1000);
  BOOST_CHECK(pNewMemBlock != nullptr);
  DWORD OldProtect = MyMemory.ProtectRegion(pNewMemBlock, PAGE_READONLY);
  BOOST_CHECK_EQUAL(OldProtect, static_cast<DWORD>(PAGE_EXECUTE_READWRITE));
  OldProtect = MyMemory.ProtectRegion(pNewMemBlock, OldProtect);
  BOOST_CHECK_EQUAL(OldProtect, static_cast<DWORD>(PAGE_READONLY));
  MyMemory.Free(pNewMemBlock);
  
  // Test AllocAndFree
  {
    HadesMem::AllocAndFree const MyAllocTest(MyMemory, 0x1000);
  }
  
  // Test MemoryMgr::FlushCache
  MyMemory.FlushCache(reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
    &TestCall)), 10);
}
  
BOOST_AUTO_TEST_CASE(ProcessInfoTest)
{
  // Create memory manager for self
  HadesMem::MemoryMgr MyMemory(GetCurrentProcessId());

  // Test MemoryMgr::GetProcessID
  BOOST_CHECK_EQUAL(MyMemory.GetProcessId(), GetCurrentProcessId());
  
  // Test MemoryMgr::GetProcessHandle
  BOOST_CHECK_EQUAL(MyMemory.GetProcessHandle(), GetCurrentProcess());
  
  // Test MemoryMgr::GetProcessPath
  BOOST_CHECK(!MyMemory.GetProcessPath().empty());
  
  // Test MemoryMgr::IsWoW64
#if defined(_M_AMD64) 
  BOOST_CHECK_EQUAL(MyMemory.IsWoW64Process(), false);
#elif defined(_M_IX86) 
  typedef BOOL (WINAPI* tIsWow64Process)(HANDLE hProcess, 
    PBOOL Wow64Process);
  auto pIsWow64Process = reinterpret_cast<tIsWow64Process>(
    GetProcAddress(GetModuleHandle(L"kernel32.dll"), "IsWow64Process"));
  if (pIsWow64Process)
  {
    BOOL Wow64Process = FALSE;
    BOOST_REQUIRE(pIsWow64Process(MyMemory.GetProcessHandle(), 
      &Wow64Process));
    BOOST_CHECK_EQUAL(MyMemory.IsWoW64Process(), 
      (Wow64Process ? true : false));
  }
#else 
#error "[HadesMem] Unsupported architecture."
#endif
}

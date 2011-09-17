// Copyright Joshua Boyce 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

// Hades
#include <HadesMemory/Detail/Process.hpp>

// Boost
#define BOOST_TEST_MODULE ProcessTest
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(ConstructorsTest)
{
  // Create process manager for self
  HadesMem::Detail::Process MyProcess(GetCurrentProcessId());
  HadesMem::Detail::Process OtherProcess(MyProcess);
  BOOST_CHECK(MyProcess == OtherProcess);
  MyProcess = OtherProcess;
  BOOST_CHECK_THROW(HadesMem::Detail::Process InvalidProc(
    static_cast<DWORD>(-1)), HadesMem::HadesMemError);
  HadesMem::Detail::Process MovedProcess(std::move(OtherProcess));
  MyProcess = std::move(MovedProcess);
  BOOST_CHECK_EQUAL(MyProcess.GetID(), GetCurrentProcessId());
}

BOOST_AUTO_TEST_CASE(ProcessInfoTest)
{
  // Create process manager for self
  HadesMem::Detail::Process MyProcess(GetCurrentProcessId());
  
  // Check process APIs for predictable values where possible, otherwise just 
  // ensure they run without exception
  BOOST_CHECK_EQUAL(MyProcess.GetHandle(), GetCurrentProcess());
  BOOST_CHECK_EQUAL(MyProcess.GetID(), GetCurrentProcessId());
  BOOST_CHECK(!MyProcess.GetPath().empty());
  
  // Test Process::IsWoW64
#if defined(_M_AMD64) 
  BOOST_CHECK_EQUAL(MyProcess.IsWoW64(), false);
#elif defined(_M_IX86) 
  typedef BOOL (WINAPI* tIsWow64Process)(HANDLE hProcess, 
    PBOOL Wow64Process);
  auto pIsWow64Process = reinterpret_cast<tIsWow64Process>(
    GetProcAddress(GetModuleHandle(L"kernel32.dll"), "IsWow64Process"));
  if (pIsWow64Process)
  {
    BOOL Wow64Process = FALSE;
    BOOST_REQUIRE(pIsWow64Process(MyProcess.GetHandle(), &Wow64Process));
    BOOST_CHECK_EQUAL(MyProcess.IsWoW64(), (Wow64Process ? true : false));
  }
#else 
#error "[HadesMem] Unsupported architecture."
#endif
}
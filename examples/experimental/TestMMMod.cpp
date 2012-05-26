// Copyright Joshua Boyce 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

// Hades
#include <HadesMemory/Memory.hpp>
#include <HadesMemory/PeLib/PeLib.hpp>
#include <HadesMemory/Detail/Config.hpp>

// Boost
#ifdef HADES_MSVC
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/exception/all.hpp>
#endif

// Windows API
#include <Windows.h>

#ifdef HADES_MSVC

// Image base linker 'trick'
EXTERN_C IMAGE_DOS_HEADER __ImageBase;

#pragma warning(push, 1)
void TestSEH2()
{
  // Test SEH
  __try 
  {
    int* pInt = 0;
    *pInt = 0;
  }
  __except (EXCEPTION_CONTINUE_SEARCH)
  { }
}

void TestSEH()
{
  // Test SEH
  __try 
  {
    TestSEH2();
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    MessageBox(nullptr, L"Testing SEH.", L"MMHelper", MB_OK);
  }
}
#pragma warning(pop)

void TestRelocs()
{
  MessageBox(nullptr, L"Testing relocations.", L"MMHelper", MB_OK);
}

void TestCPPEH()
{
  try
  {
    throw std::runtime_error("Testing C++ EH.");
  }
  catch (std::exception const& e)
  {
    MessageBoxA(nullptr, boost::diagnostic_information(e).c_str(), 
      "MMHelper", MB_OK);
  }
  catch (...)
  {
    MessageBoxA(nullptr, "Caught unknown exception.", "MMHelper", MB_OK);
  }
}

void TestTls()
{
  // FIXME: Add multi-threading test to check for TLS 'thrashing' (i.e. 
  // ensure no existing data is being overwritten).
  boost::thread_specific_ptr<std::wstring> TlsTest;
  TlsTest.reset(new std::wstring(L"Testing TLS."));
  MessageBox(nullptr, TlsTest->c_str(), L"MMHelper", MB_OK);
}

void TestTls2()
{
  double d = 3.14;
  d = fabs(d);
  char buf[20];
  ZeroMemory(&buf[0], sizeof(buf));
  sprintf_s(buf, sizeof(buf), "%f", d);
  MessageBoxA(nullptr, buf, "MMHelper", MB_OK);
}

void TestTls3()
{
  // FIXME: Check which OS versions this syntax is supported on and wrap in 
  // version detection code if needed.
  // FIXME: Add multi-threading test.
  // FIXME: Add multi-threading test to check for TLS 'thrashing' (i.e. 
  // ensure no existing data is being overwritten).
  __declspec(thread) static int i = 0;
  i = 50;
  MessageBoxW(nullptr, boost::lexical_cast<std::wstring>(i).c_str(), 
    L"MMHelper", MB_OK);
}

void TestTls4()
{
  MessageBox(nullptr, L"at_thread_exit callback", L"MMHelper", MB_OK);
}

extern "C" __declspec(dllexport) DWORD __stdcall Test(HMODULE /*Module*/)
{
  if (IsDebuggerPresent())
  {
    DebugBreak();
  }
  
  MessageBox(nullptr, L"Testing IAT.", L"MMHelper", MB_OK);
  
  TestTls();
  
  TestTls2();
  
  TestTls3();
  
  // FIXME: Callback not currently being called. Implement TLS callbacks 
  // correctly.
  boost::this_thread::at_thread_exit(&TestTls4);

  // Test relocs
  typedef void (* tTestRelocs)();
  tTestRelocs pTestRelocs = reinterpret_cast<tTestRelocs>(&TestRelocs);
  pTestRelocs();

  // Test SEH
  TestSEH();

  // Test C++ EH
  TestCPPEH();

  // Test return values
  return 1337;
}

#endif

extern "C" __declspec(dllexport) DWORD __stdcall Initialize(HMODULE /*Module*/)
{
  // Break to debugger if present
  if (IsDebuggerPresent())
  {
    DebugBreak();
  }

  // Test return values
  return 1234;
}

BOOL WINAPI DllMain(HINSTANCE /*hinstDLL*/, DWORD /*fdwReason*/, 
  LPVOID /*lpvReserved*/)
{
  // Attempt to detect memory leaks in debug mode
#ifdef _DEBUG
  int CurrentFlags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
  int NewFlags = (_CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_LEAK_CHECK_DF | 
    _CRTDBG_CHECK_ALWAYS_DF);
  _CrtSetDbgFlag(CurrentFlags | NewFlags);
#endif

  return TRUE;
}

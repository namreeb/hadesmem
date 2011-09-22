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
#include <boost/exception/all.hpp>
#endif

// Windows API
#include <Windows.h>

#ifdef HADES_MSVC

// Image base linker 'trick'
EXTERN_C IMAGE_DOS_HEADER __ImageBase;

// Fixme: This entire module is a complete mess. Rewrite to move from 'PoC' 
// quality to at least 'alpha' or 'beta' quality.

typedef EXCEPTION_DISPOSITION
(NTAPI *PEXCEPTION_ROUTINE_CUSTOM)(
  struct _EXCEPTION_RECORD *ExceptionRecord,
  PVOID EstablisherFrame,
  struct _CONTEXT *ContextRecord,
  PVOID DispatcherContext);
  
typedef struct _EXCEPTION_REGISTRATION_RECORD_CUSTOM
{
  struct _EXCEPTION_REGISTRATION_RECORD_CUSTOM *Next;
  PEXCEPTION_ROUTINE_CUSTOM Handler;
} EXCEPTION_REGISTRATION_RECORD_CUSTOM, *PEXCEPTION_REGISTRATION_RECORD_CUSTOM;

typedef struct _DISPATCHER_CONTEXT
{
  PEXCEPTION_REGISTRATION_RECORD_CUSTOM RegistrationPointer;
} DISPATCHER_CONTEXT, *PDISPATCHER_CONTEXT;

#define EXCEPTION_CHAIN_END ((PEXCEPTION_REGISTRATION_RECORD_CUSTOM)-1)

LONG CALLBACK VectoredHandler(PEXCEPTION_POINTERS ExceptionInfo)
{
#if defined(_M_AMD64) 
  ExceptionInfo;
  return EXCEPTION_CONTINUE_SEARCH;
#elif defined(_M_IX86) 
  PVOID pTeb = NtCurrentTeb();
  if (!pTeb)
  {
    MessageBox(nullptr, L"TEB pointer invalid.", L"MMHelper", MB_OK);
    return EXCEPTION_CONTINUE_SEARCH;
  }

  PEXCEPTION_REGISTRATION_RECORD_CUSTOM pExceptionList = 
    *reinterpret_cast<PEXCEPTION_REGISTRATION_RECORD_CUSTOM*>(pTeb);
  if (!pExceptionList)
  {
    MessageBox(nullptr, L"Exception list pointer invalid.", L"MMHelper", 
      MB_OK);
    return EXCEPTION_CONTINUE_SEARCH;
  }

  while (pExceptionList != EXCEPTION_CHAIN_END)
  {
    DISPATCHER_CONTEXT DispatcherContext = { 0 };

    EXCEPTION_DISPOSITION Disposition = pExceptionList->Handler(
      ExceptionInfo->ExceptionRecord, 
      pExceptionList, 
      ExceptionInfo->ContextRecord, 
      &DispatcherContext);

    switch (Disposition)
    {
    case ExceptionContinueExecution:
      return EXCEPTION_CONTINUE_EXECUTION;

    case ExceptionContinueSearch:
      return EXCEPTION_CONTINUE_SEARCH;

    case ExceptionNestedException:
    case ExceptionCollidedUnwind:
      std::abort();

    default:
      assert(!"Unknown exception disposition.");
    }

    pExceptionList = pExceptionList->Next;
  }

  return EXCEPTION_CONTINUE_SEARCH;
#else 
#error "[HadesMem] Unsupported architecture."
#endif
}

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

void InitializeSEH()
{
#if defined(_M_AMD64) 
  HadesMem::MemoryMgr MyMemory(GetCurrentProcessId());
  HadesMem::PeFile MyPeFile(MyMemory, &__ImageBase);

  HadesMem::DosHeader MyDosHeader(MyPeFile);
  HadesMem::NtHeaders MyNtHeaders(MyPeFile);

  DWORD ExceptDirSize = MyNtHeaders.GetDataDirectorySize(HadesMem::
    NtHeaders::DataDir_Exception);
  DWORD ExceptDirRva = MyNtHeaders.GetDataDirectoryVirtualAddress(
    HadesMem::NtHeaders::DataDir_Exception);
  if (!ExceptDirSize || !ExceptDirRva)
  {
    MessageBox(nullptr, L"Image has no exception directory.", 
      L"MMHelper", MB_OK);
    return;
  }

  PRUNTIME_FUNCTION pExceptDir = static_cast<PRUNTIME_FUNCTION>(
    MyPeFile.RvaToVa(ExceptDirRva));
  DWORD NumEntries = 0;
  for (PRUNTIME_FUNCTION pExceptDirTemp = pExceptDir; pExceptDirTemp->
    UnwindData; ++pExceptDirTemp)
  {
    ++NumEntries;
  }

  if (!RtlAddFunctionTable(pExceptDir, NumEntries, reinterpret_cast<DWORD_PTR>(
    &__ImageBase)))
  {
    MessageBox(nullptr, L"Could not add function table.", L"MMHelper", MB_OK);
    return;
  }
#elif defined(_M_IX86) 
  // Add VCH
  if (!AddVectoredContinueHandler(1, &VectoredHandler))
  {
    MessageBox(nullptr, L"Failed to add VCH.", L"MMHelper", MB_OK);
  }

#else 
#error "[HadesMem] Unsupported architecture."
#endif
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

extern "C" __declspec(dllexport) DWORD __stdcall Test(HMODULE /*Module*/)
{
  // Break to debugger if present
  if (IsDebuggerPresent())
  {
    DebugBreak();
  }

  // Initialize exception handling support
  InitializeSEH();

  // Test IAT
  MessageBox(nullptr, L"Testing IAT.", L"MMHelper", MB_OK);

  // Test TLS
  boost::thread_specific_ptr<std::wstring> TlsTest;
  TlsTest.reset(new std::wstring(L"Testing TLS."));
  MessageBox(nullptr, TlsTest->c_str(), L"MMHelper", MB_OK);

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

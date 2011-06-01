/*
This file is part of HadesMem.
Copyright © 2010 Cypherjb (aka Chazwazza, aka Cypher). 
<http://www.cypherjb.com/> <cypher.jb@gmail.com>

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

// Windows API
#include <crtdbg.h>
#include <Windows.h>
#include <Winternl.h>
#include <WinNT.h>

// C++ Standard Library
#include <string>
#include <iostream>
#include <exception>

// Boost
#pragma warning(push, 1)
#include <boost/format.hpp>
#include <boost/thread/tss.hpp>
#include <boost/thread/thread.hpp>
#include <boost/exception/all.hpp>
#pragma warning(pop)

// Hades
#include "Hades-Common/Logger.h"

namespace Hades
{
  namespace Windows
  {
    typedef struct _EXCEPTION_REGISTRATION_RECORD
    {
      struct _EXCEPTION_REGISTRATION_RECORD *Next;
      PEXCEPTION_ROUTINE Handler;
    } EXCEPTION_REGISTRATION_RECORD, *PEXCEPTION_REGISTRATION_RECORD;

    typedef struct _RTL_ACTIVATION_CONTEXT_STACK_FRAME
    {
      struct _RTL_ACTIVATION_CONTEXT_STACK_FRAME *Previous;
      struct _ACTIVATION_CONTEXT                 *ActivationContext;
      ULONG                                       Flags;
    } RTL_ACTIVATION_CONTEXT_STACK_FRAME, *PRTL_ACTIVATION_CONTEXT_STACK_FRAME;

    typedef struct _ACTIVATION_CONTEXT_STACK
    {
      ULONG                               Flags;
      ULONG                               NextCookieSequenceNumber;
      RTL_ACTIVATION_CONTEXT_STACK_FRAME *ActiveFrame;
      LIST_ENTRY                          FrameListCache;
    } ACTIVATION_CONTEXT_STACK, *PACTIVATION_CONTEXT_STACK;

    typedef struct _GDI_TEB_BATCH
    {
      ULONG  Offset;
      HANDLE HDC;
      ULONG  Buffer[0x136];
    } GDI_TEB_BATCH;

    typedef struct _CLIENT_ID
    {
      HANDLE UniqueProcess;
      HANDLE UniqueThread;
    } CLIENT_ID, *PCLIENT_ID;

    typedef struct _NT_TIB {
      struct _EXCEPTION_REGISTRATION_RECORD *ExceptionList;
      PVOID StackBase;
      PVOID StackLimit;
      PVOID SubSystemTib;
      union {
        PVOID FiberData;
        DWORD Version;
      } DUMMYUNIONNAME;
      PVOID ArbitraryUserPointer;
      struct _NT_TIB *Self;
    } NT_TIB,*PNT_TIB;

    typedef struct _TEB
    {
      NT_TIB          Tib;                        /* 000 */
      PVOID           EnvironmentPointer;         /* 01c */
      CLIENT_ID       ClientId;                   /* 020 */
      PVOID           ActiveRpcHandle;            /* 028 */
      PVOID           ThreadLocalStoragePointer;  /* 02c */
      PPEB            Peb;                        /* 030 */
      ULONG           LastErrorValue;             /* 034 */
      ULONG           CountOfOwnedCriticalSections;/* 038 */
      PVOID           CsrClientThread;            /* 03c */
      PVOID           Win32ThreadInfo;            /* 040 */
      ULONG           Win32ClientInfo[31];        /* 044 used for user32 private data in Wine */
      PVOID           WOW32Reserved;              /* 0c0 */
      ULONG           CurrentLocale;              /* 0c4 */
      ULONG           FpSoftwareStatusRegister;   /* 0c8 */
      PVOID           SystemReserved1[54];        /* 0cc used for kernel32 private data in Wine */
      PVOID           Spare1;                     /* 1a4 */
      LONG            ExceptionCode;              /* 1a8 */
      PACTIVATION_CONTEXT_STACK     ActivationContextStackPointer;            /* 1a8/02c8 */
      BYTE            SpareBytes1[36];            /* 1ac */
      PVOID           SystemReserved2[10];        /* 1d4 used for ntdll private data in Wine */
      GDI_TEB_BATCH   GdiTebBatch;                /* 1fc */
      ULONG           gdiRgn;                     /* 6dc */
      ULONG           gdiPen;                     /* 6e0 */
      ULONG           gdiBrush;                   /* 6e4 */
      CLIENT_ID       RealClientId;               /* 6e8 */
      HANDLE          GdiCachedProcessHandle;     /* 6f0 */
      ULONG           GdiClientPID;               /* 6f4 */
      ULONG           GdiClientTID;               /* 6f8 */
      PVOID           GdiThreadLocaleInfo;        /* 6fc */
      PVOID           UserReserved[5];            /* 700 */
      PVOID           glDispatchTable[280];        /* 714 */
      ULONG           glReserved1[26];            /* b74 */
      PVOID           glReserved2;                /* bdc */
      PVOID           glSectionInfo;              /* be0 */
      PVOID           glSection;                  /* be4 */
      PVOID           glTable;                    /* be8 */
      PVOID           glCurrentRC;                /* bec */
      PVOID           glContext;                  /* bf0 */
      ULONG           LastStatusValue;            /* bf4 */
      UNICODE_STRING  StaticUnicodeString;        /* bf8 used by advapi32 */
      WCHAR           StaticUnicodeBuffer[261];   /* c00 used by advapi32 */
      PVOID           DeallocationStack;          /* e0c */
      PVOID           TlsSlots[64];               /* e10 */
      LIST_ENTRY      TlsLinks;                   /* f10 */
      PVOID           Vdm;                        /* f18 */
      PVOID           ReservedForNtRpc;           /* f1c */
      PVOID           DbgSsReserved[2];           /* f20 */
      ULONG           HardErrorDisabled;          /* f28 */
      PVOID           Instrumentation[16];        /* f2c */
      PVOID           WinSockData;                /* f6c */
      ULONG           GdiBatchCount;              /* f70 */
      ULONG           Spare2;                     /* f74 */
      ULONG           Spare3;                     /* f78 */
      ULONG           Spare4;                     /* f7c */
      PVOID           ReservedForOle;             /* f80 */
      ULONG           WaitingOnLoaderLock;        /* f84 */
      PVOID           Reserved5[3];               /* f88 */
      PVOID          *TlsExpansionSlots;          /* f94 */
    } TEB, *PTEB;
  }
}

// Using a VEH to do exception dispatching. Necessary when the target 
// has DEP enabled.
LONG CALLBACK MyVectoredHandler(__in  PEXCEPTION_POINTERS /*ExceptionInfo*/)
{
  auto pTeb = reinterpret_cast<Hades::Windows::PTEB>(NtCurrentTeb());

  auto pSehHead = pTeb->Tib.ExceptionList;
  for(auto pSehCurrent = pSehHead; pSehCurrent != reinterpret_cast<PVOID>(-1); 
    pSehCurrent = pSehCurrent->Next)
  {
    auto LogStr((boost::wformat(L"Current SEH: 0x%p.\n") 
      %pSehCurrent->Handler).str());
    OutputDebugString(LogStr.c_str());
  }

  return EXCEPTION_CONTINUE_SEARCH;
}

extern "C" __declspec(dllexport) DWORD __stdcall Initialize(HMODULE /*Module*/)
{
  // Break to debugger if present
  if (IsDebuggerPresent())
  {
    DebugBreak();
  }
  
  // Register VEH
  if (!AddVectoredExceptionHandler(1, &MyVectoredHandler))
  {
    MessageBox(nullptr, L"AddVEH failed!", L"Hades-MMTester", MB_OK);
  }

  // Test TLS callbacks
  boost::thread_specific_ptr<DWORD> TlsTest;
  if (!TlsTest.get())
  {
    TlsTest.reset(new DWORD(0));
  }
  *TlsTest = 1022;

  // Test EH
  try
  {
    throw std::exception("Test EH!");
  }
  catch (std::exception const& e)
  {
    // Test imports
    MessageBoxA(nullptr, e.what(), "Hades-MMTester", MB_OK);
  }

  // Test imports
  MessageBox(nullptr, L"Test IAT", L"Hades-MMTester", MB_OK);

  // Test return values
  return 0;
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

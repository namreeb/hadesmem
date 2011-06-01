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

#pragma once

// Windows API
#include <Windows.h>
#include <TlHelp32.h>

// C++ Standard Library
#include <memory>
#include <string>

// Boost
#pragma warning(push, 1)
#include <boost/noncopyable.hpp>
#include <boost/algorithm/string.hpp>
#pragma warning(pop)

// Hades
#include "Error.h"
#include "Hades-Common/I18n.h"
#include "Hades-Common/EnsureCleanup.h"

namespace Hades
{
  namespace Memory
  {
    // Process exception type
    class ProcessError : public virtual HadesMemError 
    { };

    // Process managing class
    class Process : private boost::noncopyable
    {
    public:
      // Open process from process ID
      inline explicit Process(DWORD ProcID);

      // Open process from process name
      inline explicit Process(std::wstring const& ProcName);

      // Open process from window name (and optionally class)
      inline explicit Process(std::wstring const& WindowName, 
        std::wstring const* const ClassName);

      // Get process handle
      inline HANDLE GetHandle() const;
      
      // Get process ID
      inline DWORD GetID() const;

    private:
      // Open process given process id
      inline void Open(DWORD ProcID);

      // Gets the SeDebugPrivilege
      inline void GetSeDebugPrivilege();

      // Process handle
      Windows::EnsureCloseHandle m_Handle;

      // Process ID
      DWORD m_ID;
    };

    // Open process from process id
    Process::Process(DWORD ProcID) 
      : m_Handle(nullptr), 
      m_ID(ProcID) 
    {
      // Get SeDebugPrivilege
      GetSeDebugPrivilege();

      // Open process
      if (GetCurrentProcessId() == ProcID)
      {
        m_Handle = GetCurrentProcess();
      }
      else
      {
        Open(m_ID);
      }
    }

    // Open process from process name
    Process::Process(std::wstring const& ProcName) 
      : m_Handle(nullptr), 
      m_ID(0) 
    {
      // Get SeDebugPrivilege
      GetSeDebugPrivilege();

      // Grab a new snapshot of the process
      Windows::EnsureCloseHandle const Snap(CreateToolhelp32Snapshot(
        TH32CS_SNAPPROCESS, 0));
      if (Snap == INVALID_HANDLE_VALUE)
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(ProcessError() << 
          ErrorFunction("Process::Process") << 
          ErrorString("Could not get process snapshot.") << 
          ErrorCodeWin(LastError));
      }

      // Convert process name to lowercase
      std::wstring const ProcNameLower(boost::to_lower_copy(ProcName));

      // Search for process
      PROCESSENTRY32 ProcEntry = { sizeof(ProcEntry) };
      bool Found = false;
      for (BOOL MoreMods = Process32First(Snap, &ProcEntry); MoreMods; 
        MoreMods = Process32Next(Snap, &ProcEntry)) 
      {
        Found = (boost::to_lower_copy(std::wstring(ProcEntry.szExeFile)) == 
          ProcNameLower);
        if (Found)
        {
          break;
        }
      }

      // Check process was found
      if (!Found)
      {
        BOOST_THROW_EXCEPTION(ProcessError() << 
          ErrorFunction("Process::Process") << 
          ErrorString("Could not find process."));
      }

      // Open process
      m_ID = ProcEntry.th32ProcessID;
      Open(m_ID);
    }

    // Open process from window name and class
    Process::Process(std::wstring const& WindowName, 
      std::wstring const* const ClassName) 
      : m_Handle(nullptr), 
      m_ID(0) 
    {
      // Get SeDebugPrivilege
      GetSeDebugPrivilege();

      // Find window
      HWND const MyWnd = FindWindow(ClassName ? ClassName->c_str() : nullptr, 
        WindowName.c_str());
      if (!MyWnd)
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(ProcessError() << 
          ErrorFunction("Process::Process") << 
          ErrorString("Could not find window.") << 
          ErrorCodeWin(LastError));
      }

      // Get process ID from window
      GetWindowThreadProcessId(MyWnd, &m_ID);
      if (!m_ID)
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(ProcessError() << 
          ErrorFunction("Process::Process") << 
          ErrorString("Could not get process id from window.") << 
          ErrorCodeWin(LastError));
      }

      // Open process
      Open(m_ID);
    }

    // Open process given process id
    void Process::Open(DWORD ProcID)
    {
      // Open process
      m_Handle = OpenProcess(PROCESS_CREATE_THREAD | 
        PROCESS_QUERY_INFORMATION | 
        PROCESS_VM_OPERATION | 
        PROCESS_VM_READ | 
        PROCESS_VM_WRITE, 
        FALSE, 
        ProcID);
      if (!m_Handle)
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(ProcessError() << 
          ErrorFunction("Process::Open") << 
          ErrorString("Could not open process.") << 
          ErrorCodeWin(LastError));
      }

      // Get WoW64 status of self
      BOOL IsWoW64Me = FALSE;
      if (!IsWow64Process(GetCurrentProcess(), &IsWoW64Me))
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(ProcessError() << 
          ErrorFunction("Process::Open") << 
          ErrorString("Could not detect WoW64 status of current process.") << 
          ErrorCodeWin(LastError));
      }

      // If current process is not running under WoW64 then it must be a 
      // 'native' process. If the target process is a WoW64 process that 
      // must mean that our process is a different architecture to the 
      // target process, which this library currently does not 'support'.
      // Todo: Support cross-architecture process manipulation.
      if (!IsWoW64Me)
      {
        BOOL IsWoW64 = FALSE;
        if (!IsWow64Process(m_Handle, &IsWoW64))
        {
          DWORD LastError = GetLastError();
          BOOST_THROW_EXCEPTION(ProcessError() << 
            ErrorFunction("Process::Open") << 
            ErrorString("Could not detect WoW64 status of target process.") << 
            ErrorCodeWin(LastError));
        }

        if (IsWoW64)
        {
          BOOST_THROW_EXCEPTION(ProcessError() << 
            ErrorFunction("Process::Open") << 
            ErrorString("Cross-architecture process manipulation is "
              "currently unsupported."));
        }
      }
    }

    // Gets the SeDebugPrivilege
    void Process::GetSeDebugPrivilege()
    {
      // Open current process token with adjust rights
      HANDLE TempToken = 0;
      BOOL const RetVal = OpenProcessToken(GetCurrentProcess(), 
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &TempToken);
      if (!RetVal) 
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(ProcessError() << 
          ErrorFunction("Process::GetSeDebugPrivilege") << 
          ErrorString("Could not open process token.") << 
          ErrorCodeWin(LastError));
      }
      Windows::EnsureCloseHandle const Token(TempToken);

      // Get the LUID for SE_DEBUG_NAME 
      LUID Luid = { 0 }; // Locally unique identifier
      if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &Luid)) 
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(ProcessError() << 
          ErrorFunction("Process::GetSeDebugPrivilege") << 
          ErrorString("Could not look up privilege value for SeDebugName.") << 
          ErrorCodeWin(LastError));
      }
      if (Luid.LowPart == 0 && Luid.HighPart == 0) 
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(ProcessError() << 
          ErrorFunction("Process::GetSeDebugPrivilege") << 
          ErrorString("Could not get LUID for SeDebugName.") << 
          ErrorCodeWin(LastError));
      }

      // Process privileges
      TOKEN_PRIVILEGES Privileges = { 0 };
      // Set the privileges we need
      Privileges.PrivilegeCount = 1;
      Privileges.Privileges[0].Luid = Luid;
      Privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

      // Apply the adjusted privileges
      if (!AdjustTokenPrivileges(Token, FALSE, &Privileges, sizeof(Privileges), 
        NULL, NULL)) 
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(ProcessError() << 
          ErrorFunction("Process::GetSeDebugPrivilege") << 
          ErrorString("Could not adjust token privileges.") << 
          ErrorCodeWin(LastError));
      }
    }

    // Get process handle
    HANDLE Process::GetHandle() const
    {
      return m_Handle;
    }

    // Get process ID
    DWORD Process::GetID() const
    {
      return m_ID;
    }
  }
}

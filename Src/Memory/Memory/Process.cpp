/*
This file is part of HadesMem.
Copyright (C) 2010 Joshua Boyce (aka RaptorFactor, Cypherjb, Cypher, Chazwazza).
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

// Windows API
#include <Windows.h>
#include <psapi.h>
#include <TlHelp32.h>
#include <Shellapi.h>

// Boost
#pragma warning(push, 1)
#include <boost/algorithm/string.hpp>
#pragma warning(pop)

// Hades
#include "Process.hpp"
#include "HadesCommon/Pow.hpp"
#include "HadesCommon/I18n.hpp"
#include "HadesCommon/StringBuffer.hpp"

namespace Hades
{
  namespace Memory
  {
    // Open process from process id
    Process::Process(DWORD ProcID) 
      : m_Handle(nullptr), 
      m_ID(ProcID) 
    {
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
      // Grab a new snapshot of the process
      Windows::EnsureCloseSnap const Snap(CreateToolhelp32Snapshot(
        TH32CS_SNAPPROCESS, 0));
      if (Snap == INVALID_HANDLE_VALUE)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Process::Process") << 
          ErrorString("Could not get process snapshot.") << 
          ErrorCode(LastError));
      }

      // Convert process name to lowercase
      std::wstring const ProcNameLower(boost::to_lower_copy(
        ProcName));

      // Search for process
      PROCESSENTRY32 ProcEntry = { sizeof(ProcEntry) };
      bool Found = false;
      for (BOOL MoreMods = Process32First(Snap, &ProcEntry); MoreMods; 
        MoreMods = Process32Next(Snap, &ProcEntry)) 
      {
        Found = (boost::to_lower_copy(static_cast<std::wstring>(
          ProcEntry.szExeFile)) == ProcNameLower);
        if (Found)
        {
          break;
        }
      }

      // Check process was found
      if (!Found)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Process::Process") << 
          ErrorString("Could not find process."));
      }

      // Open process
      m_ID = ProcEntry.th32ProcessID;
      Open(m_ID);
    }

    // Open process from window name and class
    Process::Process(std::wstring const& WindowName, 
      std::wstring const& ClassName) 
      : m_Handle(nullptr), 
      m_ID(0) 
    {
      // Find window
      HWND const MyWnd = FindWindow(ClassName.c_str(), WindowName.c_str());
      if (!MyWnd)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Process::Process") << 
          ErrorString("Could not find window.") << 
          ErrorCode(LastError));
      }

      // Get process ID from window
      GetWindowThreadProcessId(MyWnd, &m_ID);
      if (!m_ID)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Process::Process") << 
          ErrorString("Could not get process id from window.") << 
          ErrorCode(LastError));
      }

      // Open process
      Open(m_ID);
    }

    // Copy constructor
    Process::Process(Process const& MyProcess) 
      : m_Handle(nullptr), 
      m_ID(MyProcess.m_ID)
    {
      if (m_ID == GetCurrentProcessId())
      {
        m_Handle = GetCurrentProcess();
      }
      else
      {
        Open(m_ID);
      }
    }

    // Copy assignment
    Process& Process::operator=(Process const& MyProcess)
    {
      m_ID = MyProcess.m_ID;

      if (m_ID == GetCurrentProcessId())
      {
        m_Handle = GetCurrentProcess();
      }
      else
      {
        Open(m_ID);
      }

      return *this;
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
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Process::Open") << 
          ErrorString("Could not open process.") << 
          ErrorCode(LastError));
      }

      // Get WoW64 status of self
      BOOL IsWoW64Me = FALSE;
      if (!IsWow64Process(GetCurrentProcess(), &IsWoW64Me))
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Process::Open") << 
          ErrorString("Could not detect WoW64 status of current process.") << 
          ErrorCode(LastError));
      }

      // Get WoW64 status of target process
      BOOL IsWoW64 = FALSE;
      if (!IsWow64Process(m_Handle, &IsWoW64))
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Process::Open") << 
          ErrorString("Could not detect WoW64 status of target process.") << 
          ErrorCode(LastError));
      }
      
      // Set WoW64 status
      m_IsWoW64 = (IsWoW64 != FALSE);

      // Disable x86 -> x64 process manipulation
      if (IsWoW64Me && !IsWoW64)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Process::Open") << 
          ErrorString("x86 -> x64 process manipulation is currently "
          "unsupported."));
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
      
    // Get process path
    boost::filesystem::path Process::GetPath() const
    {
      // Note: The QueryFullProcessImageName API is more efficient and 
      // reliable but is only available on Vista+.
      DWORD const PathSize = 32767;
      std::wstring Path;
      if (!GetModuleFileNameEx(m_Handle, NULL, Util::MakeStringBuffer(Path, 
        PathSize), PathSize))
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Process::GetPath") << 
          ErrorString("Could not get path.") << 
          ErrorCode(LastError));
      }
      
      return Path;
    }
    
    // Is WoW64 process
    bool Process::IsWoW64() const
    {
      return m_IsWoW64;
    }
    
    // Create process
    Process CreateProcess(boost::filesystem::path const& Path, 
      boost::filesystem::path const& Params, 
      boost::filesystem::path const& WorkingDir)
    {
      // Start process
      SHELLEXECUTEINFO ExecInfo = { sizeof(ExecInfo) };
      ExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
      ExecInfo.lpFile = Path.empty() ? NULL : Path.c_str();
      ExecInfo.lpParameters = Params.empty() ? NULL : Params.c_str();
      ExecInfo.lpDirectory = WorkingDir.empty() ? NULL : WorkingDir.c_str();
      ExecInfo.nShow = SW_SHOWNORMAL;
      if (!ShellExecuteEx(&ExecInfo))
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Process::Error() << 
          ErrorFunction("CreateProcess") << 
          ErrorString("Could not create process.") << 
          ErrorCode(LastError));
      }
      
      // Ensure handle is closed
      Windows::EnsureCloseHandle const MyProc(ExecInfo.hProcess);
      
      // Return process object
      return Process(GetProcessId(MyProc));
    }

    // Gets the SeDebugPrivilege
    void GetSeDebugPrivilege()
    {
      // Open current process token with adjust rights
      HANDLE TempToken = 0;
      BOOL const RetVal = OpenProcessToken(GetCurrentProcess(), 
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &TempToken);
      if (!RetVal) 
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Process::Error() << 
          ErrorFunction("GetSeDebugPrivilege") << 
          ErrorString("Could not open process token.") << 
          ErrorCode(LastError));
      }
      Windows::EnsureCloseHandle const Token(TempToken);

      // Get the LUID for SE_DEBUG_NAME 
      LUID Luid = { 0 }; // Locally unique identifier
      if (!LookupPrivilegeValue(nullptr, SE_DEBUG_NAME, &Luid)) 
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Process::Error() << 
          ErrorFunction("GetSeDebugPrivilege") << 
          ErrorString("Could not look up privilege value for SeDebugName.") << 
          ErrorCode(LastError));
      }
      if (Luid.LowPart == 0 && Luid.HighPart == 0) 
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Process::Error() << 
          ErrorFunction("GetSeDebugPrivilege") << 
          ErrorString("Could not get LUID for SeDebugName.") << 
          ErrorCode(LastError));
      }

      // Process privileges
      TOKEN_PRIVILEGES Privileges = { 0 };
      // Set the privileges we need
      Privileges.PrivilegeCount = 1;
      Privileges.Privileges[0].Luid = Luid;
      Privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

      // Apply the adjusted privileges
      if (!AdjustTokenPrivileges(Token, FALSE, &Privileges, sizeof(Privileges), 
        nullptr, nullptr)) 
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Process::Error() << 
          ErrorFunction("GetSeDebugPrivilege") << 
          ErrorString("Could not adjust token privileges.") << 
          ErrorCode(LastError));
      }
      
      // Ensure privileges were adjusted
      if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Process::Error() << 
          ErrorFunction("GetSeDebugPrivilege") << 
          ErrorString("Could not assign all privileges.") << 
          ErrorCode(LastError));
      }
    }
  }
}

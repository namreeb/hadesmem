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

#pragma once

// C++ Standard Library
#include <string>

// Boost
#pragma warning(push, 1)
#include <boost/optional.hpp>
#include <boost/filesystem.hpp>
#include <boost/iterator/iterator_facade.hpp>
#pragma warning(pop)

// Windows API
#include <Windows.h>
#include <TlHelp32.h>

// Hades
#include "Fwd.hpp"
#include "Error.hpp"
#include "HadesCommon/EnsureCleanup.hpp"

namespace Hades
{
  namespace Memory
  {
    // Process managing class
    class Process
    {
    public:
      // Process exception type
      class Error : public virtual HadesMemError 
      { };

      // Open process from process ID
      explicit Process(DWORD ProcID);

      // Open process from process name
      explicit Process(std::wstring const& ProcName);

      // Open process from window name and class
      Process(std::wstring const& WindowName, std::wstring const& ClassName);

      // Copy constructor
      Process(Process const& MyProcess);

      // Copy assignment
      Process& operator=(Process const& MyProcess);

      // Move constructor
      Process(Process&& MyProcess);

      // Move assignment
      Process& operator=(Process&& MyProcess);

      // Get process handle
      HANDLE GetHandle() const;
      
      // Get process ID
      DWORD GetID() const;
      
      // Get process path
      boost::filesystem::path GetPath() const;

    private:
      // Open process given process id
      void Open(DWORD ProcID);

      // Process handle
      Windows::EnsureCloseHandle m_Handle;

      // Process ID
      DWORD m_ID;
    };
    
    // Create process
    Process CreateProcess(boost::filesystem::path const& Path, 
      boost::filesystem::path const& Params, 
      boost::filesystem::path const& WorkingDir);

    // Gets the SeDebugPrivilege
    void GetSeDebugPrivilege();

    // Process iterator
    // Fixme: Implement in a more rhobust manner. Currently just 'skips' 
    // processes if something goes wrong...
    class ProcessIter : public boost::iterator_facade<ProcessIter, 
      boost::optional<Process>, boost::incrementable_traversal_tag>, 
      private boost::noncopyable
    {
    public:
      // Constructor
      ProcessIter() 
        : m_Snap(), 
        m_Current()
      {
        // Grab a new snapshot of the process
        m_Snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (m_Snap == INVALID_HANDLE_VALUE)
        {
          std::error_code const LastError = GetLastErrorCode();
          BOOST_THROW_EXCEPTION(Process::Error() << 
            ErrorFunction("ProcessIter::First") << 
            ErrorString("Could not get process snapshot.") << 
            ErrorCode(LastError));
        }

        // Get first module entry
        PROCESSENTRY32 ProcEntry = { sizeof(ProcEntry) };
        if (!Process32First(m_Snap, &ProcEntry))
        {
          std::error_code const LastError = GetLastErrorCode();
          BOOST_THROW_EXCEPTION(Process::Error() << 
            ErrorFunction("ProcessIter::First") << 
            ErrorString("Error enumerating process list.") << 
            ErrorCode(LastError));
        }
        
        // Get WoW64 status of self
        BOOL IsWoW64Me = FALSE;
        if (!IsWow64Process(GetCurrentProcess(), &IsWoW64Me))
        {
          std::error_code const LastError = GetLastErrorCode();
          BOOST_THROW_EXCEPTION(Process::Error() << 
            ErrorFunction("ProcessIter::First") << 
            ErrorString("Could not detect WoW64 status of current process.") << 
            ErrorCode(LastError));
        }
        
        // Get handle to target
        // Using more access flags than is strictly necessary, but if I don't 
        // it will simply fail further down the chain
        Windows::EnsureCloseHandle TargetProc(OpenProcess(PROCESS_CREATE_THREAD | 
          PROCESS_QUERY_INFORMATION | 
          PROCESS_VM_OPERATION | 
          PROCESS_VM_READ | 
          PROCESS_VM_WRITE, 
          FALSE, ProcEntry.th32ProcessID));
        if (!TargetProc)
        {
          increment();
          return;
        }
  
        // Get WoW64 status of target process
        BOOL IsWoW64 = FALSE;
        if (!IsWow64Process(TargetProc, &IsWoW64))
        {
          std::error_code const LastError = GetLastErrorCode();
          BOOST_THROW_EXCEPTION(Process::Error() << 
            ErrorFunction("ProcessIter::First") << 
            ErrorString("Could not detect WoW64 status of target process.") << 
            ErrorCode(LastError));
        }
  
        // Ensure WoW64 status of both self and target match
        if (IsWoW64Me != IsWoW64)
        {
          increment();
        }
        else
        {
          m_Current = Process(ProcEntry.th32ProcessID);
        }
      }

    private:
      // Allow Boost.Iterator access to internals
      friend class boost::iterator_core_access;

      // For Boost.Iterator
      void increment() 
      {
        PROCESSENTRY32 ProcEntry = { sizeof(ProcEntry) };
        if (!Process32Next(m_Snap, &ProcEntry))
        {
          if (GetLastError() != ERROR_NO_MORE_FILES)
          {
            std::error_code const LastError = GetLastErrorCode();
            BOOST_THROW_EXCEPTION(Process::Error() << 
              ErrorFunction("ProcessIter::Next") << 
              ErrorString("Error enumerating process list.") << 
              ErrorCode(LastError));
          }

          m_Current = boost::optional<Process>();
        }
        else
        {
          // Get WoW64 status of self
          BOOL IsWoW64Me = FALSE;
          if (!IsWow64Process(GetCurrentProcess(), &IsWoW64Me))
          {
            std::error_code const LastError = GetLastErrorCode();
            BOOST_THROW_EXCEPTION(Process::Error() << 
              ErrorFunction("ProcessIter::Next") << 
              ErrorString("Could not detect WoW64 status of current "
                "process.") << 
              ErrorCode(LastError));
          }
        
          // Get handle to target
          // Using more access flags than is strictly necessar, but if I don't 
          // it will simply fail further down the chain
          Windows::EnsureCloseHandle TargetProc(OpenProcess(PROCESS_CREATE_THREAD | 
            PROCESS_QUERY_INFORMATION | 
            PROCESS_VM_OPERATION | 
            PROCESS_VM_READ | 
            PROCESS_VM_WRITE, 
            FALSE, ProcEntry.th32ProcessID));
          if (!TargetProc)
          {
            increment();
            return;
          }
    
          // Get WoW64 status of target process
          BOOL IsWoW64 = FALSE;
          if (!IsWow64Process(TargetProc, &IsWoW64))
          {
            std::error_code const LastError = GetLastErrorCode();
            BOOST_THROW_EXCEPTION(Process::Error() << 
              ErrorFunction("ProcessIter::Next") << 
              ErrorString("Could not detect WoW64 status of target "
                "process.") << 
              ErrorCode(LastError));
          }
  
          // Ensure WoW64 status of both self and target match
          if (IsWoW64Me != IsWoW64)
          {
            increment();
          }
          else
          {
            m_Current = Process(ProcEntry.th32ProcessID);
          }
        }
      }

      // For Boost.Iterator
      boost::optional<Process>& dereference() const
      {
        return m_Current;
      }

      // Toolhelp32 snapshot handle
      Windows::EnsureCloseSnap m_Snap;

      // Current module
      mutable boost::optional<Process> m_Current;
    };
  }
}

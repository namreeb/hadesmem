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

#pragma once

// Hades
#include <HadesMemory/Fwd.hpp>
#include <HadesMemory/Error.hpp>
#include <HadesCommon/EnsureCleanup.hpp>

// C++ Standard Library
#include <string>

// Boost
#include <boost/filesystem.hpp>

// Windows API
#include <Windows.h>

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
      Process(Process const& Other);
    
      // Copy assignment
      Process& operator=(Process const& Other);
    
      // Get process handle
      HANDLE GetHandle() const;
      
      // Get process ID
      DWORD GetID() const;
      
      // Get process path
      std::wstring GetPath() const;
        
      // Is WoW64 process
      bool IsWoW64() const;
    
    private:
      // Get WoW64 status of process and set member var
      void SetWoW64();
      
      // Open process given process id
      void Open(DWORD ProcID);
    
      // Process handle
      Windows::EnsureCloseHandle m_Handle;
    
      // Process ID
      DWORD m_ID;
      
      // Is WoW64 process
      bool m_IsWoW64;
    };
    
    // Create process
    Process CreateProcess(std::wstring const& Path, 
      std::wstring const& Params, 
      std::wstring const& WorkingDir);
    
    // Gets the SeDebugPrivilege
    void GetSeDebugPrivilege();
  } // namespace Memory
} // namespace Hades

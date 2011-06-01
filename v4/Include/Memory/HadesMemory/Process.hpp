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

// C++ Standard Library
#include <string>

// Boost
#pragma warning(push, 1)
#include <boost/filesystem.hpp>
#pragma warning(pop)

// Windows API
#include <Windows.h>

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
        
      // Is WoW64 process
      bool IsWoW64() const;

    private:
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
    Process CreateProcess(boost::filesystem::path const& Path, 
      boost::filesystem::path const& Params, 
      boost::filesystem::path const& WorkingDir);

    // Gets the SeDebugPrivilege
    void GetSeDebugPrivilege();
  }
}

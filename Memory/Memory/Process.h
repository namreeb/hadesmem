/*
This file is part of HadesMem.
Copyright © 2010 RaptorFactor (aka Cypherjb, Cypher, Chazwazza). 
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

// Windows API
#include <tchar.h>
#include <Windows.h>
#include <TlHelp32.h>

// C++ Standard Library
#include <string>

// Boost
#pragma warning(push, 1)
#include <boost/noncopyable.hpp>
#pragma warning(pop)

// Hades
#include "Fwd.h"
#include "Error.h"
#include "Common/EnsureCleanup.h"

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
      explicit Process(std::basic_string<TCHAR> const& ProcName);

      // Open process from window name and class
      Process(std::basic_string<TCHAR> const& WindowName, 
        std::basic_string<TCHAR> const& ClassName);

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

    private:
      // Open process given process id
      void Open(DWORD ProcID);

      // Gets the SeDebugPrivilege
      void GetSeDebugPrivilege();

      // Process handle
      Windows::EnsureCloseHandle m_Handle;

      // Process ID
      DWORD m_ID;
    };
  }
}

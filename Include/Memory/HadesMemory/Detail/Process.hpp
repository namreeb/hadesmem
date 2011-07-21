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
#include <HadesMemory/Detail/Error.hpp>

// C++ Standard Library
#include <string>
#include <memory>

// Windows API
#include <Windows.h>

namespace HadesMem
{
  namespace Detail
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
      
      // Copy constructor
      Process(Process const& Other);
      
      // Copy assignment operator
      Process& operator=(Process const& Other);
      
      // Move constructor
      Process(Process&& Other);
      
      // Move assignment operator
      Process& operator=(Process&& Other);
      
      // Destructor
      ~Process();
    
      // Get process handle
      HANDLE GetHandle() const;
      
      // Get process ID
      DWORD GetID() const;
      
      // Get process path
      std::wstring GetPath() const;
      
      // Is WoW64 process
      bool IsWoW64() const;
      
      // Equality operator
      bool operator==(Process const& Rhs) const;
      
      // Inequality operator
      bool operator!=(Process const& Rhs) const;
    
    private:
      // Implementation
      class Impl;
      std::shared_ptr<Impl> m_pImpl;
    };
  } // namespace Detail
} // namespace HadesMem

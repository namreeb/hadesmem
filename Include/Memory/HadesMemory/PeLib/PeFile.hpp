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
#include <HadesMemory/Detail/Fwd.hpp>
#include <HadesMemory/Detail/Error.hpp>
#include <HadesMemory/MemoryMgr.hpp>

// Windows
#include <Windows.h>

namespace HadesMem
{
  // PE file format wrapper
  class PeFile
  {
  public:
    // PeFile exception type
    class Error : public virtual HadesMemError 
    { };

    enum FileType
    {
      FileType_Image, 
      FileType_Data
    };

    // Constructor
    PeFile(MemoryMgr const& MyMemory, PVOID Address, 
      FileType Type = FileType_Image);
      
    // Copy constructor
    PeFile(PeFile const& Other);
    
    // Copy assignment operator
    PeFile& operator=(PeFile const& Other);
    
    // Move constructor
    PeFile(PeFile&& Other);
    
    // Move assignment operator
    PeFile& operator=(PeFile&& Other);
    
    // Destructor
    ~PeFile();

    // Get memory manager
    MemoryMgr GetMemoryMgr() const;

    // Get base address
    PVOID GetBase() const;

    // Convert RVA to VA
    PVOID RvaToVa(DWORD Rva) const;

    // Get file type
    FileType GetType() const;
    
    // Equality operator
    bool operator==(PeFile const& Rhs) const;
    
    // Inequality operator
    bool operator!=(PeFile const& Rhs) const;

  protected:
    // Memory instance
    MemoryMgr m_Memory;

    // Base address
    PBYTE m_pBase;

    // File type
    FileType m_Type;
  };
}

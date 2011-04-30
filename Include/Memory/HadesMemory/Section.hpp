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

// Windows
#include <Windows.h>

// Hades
#include "Fwd.hpp"
#include "Error.hpp"
#include "PeFile.hpp"
#include "MemoryMgr.hpp"

namespace Hades
{
  namespace Memory
  {
    // PE file section
    class Section
    {
    public:
      // Section error class
      class Error : public virtual HadesMemError
      { };

      // Constructor
      Section(PeFile const& MyPeFile, WORD Number);

      // Get name
      std::string GetName() const;

      // Set name
      void SetName(std::string const& Name) const;

      // Get virtual address
      DWORD GetVirtualAddress() const;

      // Set virtual address
      void SetVirtualAddress(DWORD VirtualAddress) const;

      // Get virtual size
      DWORD GetVirtualSize() const;

      // Set virtual size
      void SetVirtualSize(DWORD VirtualSize) const;

      // Get size of raw data
      DWORD GetSizeOfRawData() const;

      // Set size of raw data
      void SetSizeOfRawData(DWORD SizeOfRawData) const;

      // Get pointer to raw data
      DWORD GetPointerToRawData() const;

      // Set pointer to raw data
      void SetPointerToRawData(DWORD PointerToRawData) const;

      // Get pointer to relocations
      DWORD GetPointerToRelocations() const;

      // Set pointer to relocations
      void SetPointerToRelocations(DWORD PointerToRelocations) const;

      // Get pointer to line numbers
      DWORD GetPointerToLinenumbers() const;

      // Set pointer to line numbers
      void SetPointerToLinenumbers(DWORD PointerToLinenumbers) const;

      // Get number of relocations
      WORD GetNumberOfRelocations() const;

      // Set number of relocations
      void SetNumberOfRelocations(WORD NumberOfRelocations) const;

      // Get number of line numbers
      WORD GetNumberOfLinenumbers() const;

      // Set number of line numbers
      void SetNumberOfLinenumbers(WORD NumberOfLinenumbers) const;

      // Get characteristics
      DWORD GetCharacteristics() const;

      // Set characteristics
      void SetCharacteristics(DWORD Characteristics) const;

      // Get section header base
      PBYTE GetBase() const;

      // Get raw section header
      IMAGE_SECTION_HEADER GetSectionHeaderRaw() const;
      
      // Get section number
      WORD GetNumber() const;

    private:
      // PE file
      PeFile m_PeFile;

      // Memory instance
      MemoryMgr m_Memory;

      // Section number
      WORD m_SectionNum;
    };
  }
}

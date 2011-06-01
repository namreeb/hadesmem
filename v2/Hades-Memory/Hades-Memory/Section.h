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

// Windows
#include <Windows.h>

// C++ Standard Library
#include <string>

// Boost
#pragma warning(push, 1)
#pragma warning (disable: ALL_CODE_ANALYSIS_WARNINGS)
#include <boost/noncopyable.hpp>
#pragma warning(pop)

// Hades
#include "Fwd.h"
#include "Error.h"
#include "PeFile.h"
#include "MemoryMgr.h"

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

      // Get virtual address
      DWORD GetVirtualAddress() const;

      // Get virtual size
      DWORD GetVirtualSize() const;

      // Get size of raw data
      DWORD GetSizeOfRawData() const;

      // Get pointer to raw data
      DWORD GetPointerToRawData() const;

      // Get pointer to relocations
      DWORD GetPointerToRelocations() const;

      // Get pointer to line numbers
      DWORD GetPointerToLinenumbers() const;

      // Get number of relocations
      WORD GetNumberOfRelocations() const;

      // Get number of line numbers
      WORD GetNumberOfLinenumbers() const;

      // Get characteristics
      DWORD GetCharacteristics() const;

      // Get section header base
      PBYTE GetBase() const;

      // Get raw section header
      IMAGE_SECTION_HEADER GetSectionHeaderRaw() const;

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

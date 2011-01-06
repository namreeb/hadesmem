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
#include <array>

// Hades
#include "Fwd.hpp"
#include "Error.hpp"
#include "PeFile.hpp"
#include "MemoryMgr.hpp"

namespace Hades
{
  namespace Memory
  {
    // PE file DOS header
    class DosHeader
    {
    public:
      // DOS header error class
      class Error : public virtual HadesMemError
      { };

      // Constructor
      explicit DosHeader(PeFile const& MyPeFile);

      // Whether magic is valid
      bool IsMagicValid() const;

      // Ensure magic is valid
      void EnsureMagicValid() const;

      // Get magic
      WORD GetMagic() const;

      // Get bytes on last page
      WORD GetBytesOnLastPage() const;

      // Get pages in file
      WORD GetPagesInFile() const;

      // Get relocations
      WORD GetRelocations() const;

      // Get size of header in paragraphs
      WORD GetSizeOfHeaderInParagraphs() const;

      // Get minimum extra paragraphs needed
      WORD GetMinExtraParagraphs() const;

      // Get maximum extra paragraphs needed
      WORD GetMaxExtraParagraphs() const;

      // Get initial SS value
      WORD GetInitialSS() const;

      // Get initial SP value
      WORD GetInitialSP() const;

      // Get checksum
      WORD GetChecksum() const;

      // Get initial IP value
      WORD GetInitialIP() const;

      // Get initial CS value
      WORD GetInitialCS() const;

      // Get file address of reloc table
      WORD GetRelocTableFileAddr() const;

      // Get overlay number
      WORD GetOverlayNum() const;

      // Get first set of reserved words
      std::array<WORD, 4> GetReservedWords1() const;

      // Get OEM ID
      WORD GetOEMID() const;

      // Get OEM info
      WORD GetOEMInfo() const;

      // Get second set of reserved words
      std::array<WORD, 10> GetReservedWords2() const;

      // Get new header offset
      LONG GetNewHeaderOffset() const;

      // Set magic
      void SetMagic(WORD Magic) const;

      // Set bytes on last page
      void SetBytesOnLastPage(WORD BytesOnLastPage) const;

      // Set pages in file
      void SetPagesInFile(WORD PagesInFile) const;

      // Set relocations
      void SetRelocations(WORD Relocations) const;

      // Set size of header in paragraphs
      void SetSizeOfHeaderInParagraphs(WORD SizeOfHeaderInParagraphs) const;

      // Set minimum extra paragraphs needed
      void SetMinExtraParagraphs(WORD MinExtraParagraphs) const;

      // Set maximum extra paragraphs needed
      void SetMaxExtraParagraphs(WORD MaxExtraParagraphs) const;

      // Set initial SS value
      void SetInitialSS(WORD InitialSS) const;

      // Set initial SP value
      void SetInitialSP(WORD InitialSP) const;

      // Set checksum
      void SetChecksum(WORD Checksum) const;

      // Set initial IP value
      void SetInitialIP(WORD InitialIP) const;

      // Set initial CS value
      void SetInitialCS(WORD InitialCS) const;

      // Set file address of reloc table
      void SetRelocTableFileAddr(WORD RelocTableFileAddr) const;

      // Set overlay number
      void SetOverlayNum(WORD OverlayNum) const;

      // Set first set of reserved words
      void SetReservedWords1(std::array<WORD, 4> const& ReservedWords1) const;

      // Set OEM ID
      void SetOEMID(WORD OEMID) const;

      // Set OEM info
      void SetOEMInfo(WORD OEMInfo) const;

      // Set second set of reserved words
      void SetReservedWords2(std::array<WORD, 10> const& ReservedWords2) const;

      // Set new header offset
      void SetNewHeaderOffset(LONG Offset) const;

    private:
      // PE file
      PeFile m_PeFile;

      // Memory instance
      MemoryMgr m_Memory;

      // DOS header base
      PBYTE m_pBase;
    };
  }
}

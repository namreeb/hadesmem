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

// Hades
#include "PeFile.h"
#include "MemoryMgr.h"
#include "DosHeader.h"

namespace Hades
{
  namespace Memory
  {
    // Constructor
    DosHeader::DosHeader(PeFile const& MyPeFile)
      : m_PeFile(MyPeFile), 
      m_Memory(m_PeFile.GetMemoryMgr()), 
      m_pBase(m_PeFile.GetBase())
    {
      // Ensure magic is valid
      EnsureMagicValid();
    }

    // Whether magic is valid
    bool DosHeader::IsMagicValid() const
    {
      return IMAGE_DOS_SIGNATURE == GetMagic();
    }

    // Ensure magic is valid
    void DosHeader::EnsureMagicValid() const
    {
      if (!IsMagicValid())
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("DosHeader::EnsureSignatureValid") << 
          ErrorString("DOS header magic invalid."));
      }
    }

    // Get magic
    WORD DosHeader::GetMagic() const
    {
      return m_Memory.Read<WORD>(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, 
        e_magic));
    }

    // Get bytes on last page
    WORD DosHeader::GetBytesOnLastPage() const
    {
      return m_Memory.Read<WORD>(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, 
        e_cblp));
    }

    // Get pages in file
    WORD DosHeader::GetPagesInFile() const
    {
      return m_Memory.Read<WORD>(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, 
        e_cp));

    }

    // Get relocations
    WORD DosHeader::GetRelocations() const
    {
      return m_Memory.Read<WORD>(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, 
        e_crlc));
    }

    // Get size of header in paragraphs
    WORD DosHeader::GetSizeOfHeaderInParagraphs() const
    {
      return m_Memory.Read<WORD>(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, 
        e_cparhdr));
    }

    // Get minimum extra paragraphs needed
    WORD DosHeader::GetMinExtraParagraphs() const
    {
      return m_Memory.Read<WORD>(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, 
        e_minalloc));
    }

    // Get maximum extra paragraphs needed
    WORD DosHeader::GetMaxExtraParagraphs() const
    {
      return m_Memory.Read<WORD>(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, 
        e_maxalloc));
    }

    // Get initial SS value
    WORD DosHeader::GetInitialSS() const
    {
      return m_Memory.Read<WORD>(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, 
        e_ss));
    }

    // Get initial SP value
    WORD DosHeader::GetInitialSP() const
    {
      return m_Memory.Read<WORD>(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, 
        e_sp));
    }

    // Get checksum
    WORD DosHeader::GetChecksum() const
    {
      return m_Memory.Read<WORD>(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, 
        e_csum));
    }

    // Get initial IP value
    WORD DosHeader::GetInitialIP() const
    {
      return m_Memory.Read<WORD>(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, 
        e_ip));
    }

    // Get initial CS value
    WORD DosHeader::GetInitialCS() const
    {
      return m_Memory.Read<WORD>(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, 
        e_cs));
    }

    // Get file address of reloc table
    WORD DosHeader::GetRelocTableFileAddr() const
    {
      return m_Memory.Read<WORD>(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, 
        e_lfarlc));
    }

    // Get overlay number
    WORD DosHeader::GetOverlayNum() const
    {
      return m_Memory.Read<WORD>(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, 
        e_ovno));
    }

    // Get first set of reserved words
    std::array<WORD, 4> DosHeader::GetReservedWords1() const
    {
      return m_Memory.Read<std::array<WORD, 4>>(m_pBase + FIELD_OFFSET(
        IMAGE_DOS_HEADER, e_res));
    }

    // Get OEM ID
    WORD DosHeader::GetOEMID() const
    {
      return m_Memory.Read<WORD>(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, 
        e_oemid));
    }

    // Get OEM info
    WORD DosHeader::GetOEMInfo() const
    {
      return m_Memory.Read<WORD>(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, 
        e_oeminfo));
    }

    // Get second set of reserved words
    std::array<WORD, 10> DosHeader::GetReservedWords2() const
    {
      return m_Memory.Read<std::array<WORD, 10>>(m_pBase + FIELD_OFFSET(
        IMAGE_DOS_HEADER, e_res2));
    }

    // Get new header offset
    LONG DosHeader::GetNewHeaderOffset() const
    {
      return m_Memory.Read<LONG>(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, 
        e_lfanew));
    }

    // Set magic
    void DosHeader::SetMagic(WORD Magic) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, e_magic), 
        Magic);

      if (GetMagic() != Magic)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("DosHeader::SetMagic") << 
          ErrorString("Could not set magic. Verification mismatch."));
      }
    }

    // Set bytes on last page
    void DosHeader::SetBytesOnLastPage(WORD BytesOnLastPage) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, e_cblp), 
        BytesOnLastPage);

      if (GetBytesOnLastPage() != BytesOnLastPage)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("DosHeader::SetBytesOnLastPage") << 
          ErrorString("Could not set bytes on last page. Verification "
          "mismatch."));
      }
    }

    // Set pages in file
    void DosHeader::SetPagesInFile(WORD PagesInFile) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, e_cp), 
        PagesInFile);

      if (GetPagesInFile() != PagesInFile)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("DosHeader::SetPagesInFile") << 
          ErrorString("Could not set pages in file. Verification mismatch."));
      }
    }

    // Set relocations
    void DosHeader::SetRelocations(WORD Relocations) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, e_crlc), 
        Relocations);

      if (GetRelocations() != Relocations)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("DosHeader::SetRelocations") << 
          ErrorString("Could not set relocations. Verification mismatch."));
      }
    }

    // Set size of header in paragraphs
    void DosHeader::SetSizeOfHeaderInParagraphs(WORD SizeOfHeaderInParagraphs) 
      const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, e_cparhdr), 
        SizeOfHeaderInParagraphs);

      if (GetSizeOfHeaderInParagraphs() != SizeOfHeaderInParagraphs)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("DosHeader::SetSizeOfHeaderInParagraphs") << 
          ErrorString("Could not set size of header in paragraphs. "
          "Verification mismatch."));
      }
    }

    // Set min extra paragraphs
    void DosHeader::SetMinExtraParagraphs(WORD MinExtraParagraphs) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, e_minalloc), 
        MinExtraParagraphs);

      if (GetMinExtraParagraphs() != MinExtraParagraphs)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("DosHeader::SetMinExtraParagraphs") << 
          ErrorString("Could not set min extra paragraphs. Verification "
          "mismatch."));
      }
    }

    // Set max extra paragraphs
    void DosHeader::SetMaxExtraParagraphs(WORD MaxExtraParagraphs) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, e_maxalloc), 
        MaxExtraParagraphs);

      if (GetMaxExtraParagraphs() != MaxExtraParagraphs)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("DosHeader::SetMaxExtraParagraphs") << 
          ErrorString("Could not set max extra paragraphs. Verification "
          "mismatch."));
      }
    }

    // Set initial SS
    void DosHeader::SetInitialSS(WORD InitialSS) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, e_ss), 
        InitialSS);

      if (GetInitialSS() != InitialSS)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("DosHeader::SetInitialSS") << 
          ErrorString("Could not set initial SS. Verification mismatch."));
      }
    }

    // Set initial SP
    void DosHeader::SetInitialSP(WORD InitialSP) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, e_sp), 
        InitialSP);

      if (GetInitialSP() != InitialSP)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("DosHeader::SetInitialSP") << 
          ErrorString("Could not set initial SP. Verification mismatch."));
      }
    }

    // Set checksum
    void DosHeader::SetChecksum(WORD Checksum) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, e_csum), 
        Checksum);

      if (GetChecksum() != Checksum)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("DosHeader::SetChecksum") << 
          ErrorString("Could not set checksum. Verification mismatch."));
      }
    }

    // Set initial IP
    void DosHeader::SetInitialIP(WORD InitialIP) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, e_ip), 
        InitialIP);

      if (GetInitialIP() != InitialIP)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("DosHeader::SetInitialIP") << 
          ErrorString("Could not set initial IP. Verification mismatch."));
      }
    }

    // Set initial CS
    void DosHeader::SetInitialCS(WORD InitialCS) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, e_cs), 
        InitialCS);

      if (GetInitialCS() != InitialCS)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("DosHeader::SetInitialCS") << 
          ErrorString("Could not set initial CS. Verification mismatch."));
      }
    }

    // Set reloc table file address
    void DosHeader::SetRelocTableFileAddr(WORD RelocTableFileAddr) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, e_lfarlc), 
        RelocTableFileAddr);

      if (GetRelocTableFileAddr() != RelocTableFileAddr)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("DosHeader::SetRelocTableFileAddr") << 
          ErrorString("Could not set reloc table file address. Verification "
          "mismatch."));
      }
    }

    // Set overlay number
    void DosHeader::SetOverlayNum(WORD OverlayNum) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, e_ovno), 
        OverlayNum);

      if (GetOverlayNum() != OverlayNum)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("DosHeader::SetOverlayNum") << 
          ErrorString("Could not set overlay number. Verification mismatch."));
      }
    }

    // Set first set of reserved words
    void DosHeader::SetReservedWords1(std::array<WORD, 4> const& 
      ReservedWords1) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, e_res), 
        ReservedWords1);

      if (GetReservedWords1() != ReservedWords1)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("DosHeader::SetReservedWords1") << 
          ErrorString("Could not set first set of reserved words. "
          "Verification mismatch."));
      }
    }

    // Set OEM ID
    void DosHeader::SetOEMID(WORD OEMID) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, e_oemid), 
        OEMID);

      if (GetOEMID() != OEMID)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("DosHeader::SetOEMID") << 
          ErrorString("Could not set OEM ID. Verification mismatch."));
      }
    }

    // Set OEM info
    void DosHeader::SetOEMInfo(WORD OEMInfo) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, e_oeminfo), 
        OEMInfo);

      if (GetOEMInfo() != OEMInfo)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("DosHeader::SetOEMInfo") << 
          ErrorString("Could not set OEM info. Verification mismatch."));
      }
    }

    // Set second set of reserved words
    void DosHeader::SetReservedWords2(std::array<WORD, 10> const& 
      ReservedWords2) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, e_res2), 
        ReservedWords2);

      if (GetReservedWords2() != ReservedWords2)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("DosHeader::SetReservedWords2") << 
          ErrorString("Could not set second set of reserved words. "
          "Verification mismatch."));
      }
    }

    // Set new header offset
    void DosHeader::SetNewHeaderOffset(LONG Offset) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_DOS_HEADER, e_lfanew), 
        Offset);

      if (GetNewHeaderOffset() != Offset)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("DosHeader::SetNewHeaderOffset") << 
          ErrorString("Could not set new header offset. Verification "
          "mismatch."));
      }
    }
  }
}

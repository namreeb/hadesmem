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

// Hades
#include "PeFile.hpp"
#include "MemoryMgr.hpp"

namespace Hades
{
  namespace Memory
  {
    // Constructor
    PeFile::PeFile(MemoryMgr const& MyMemory, PVOID Address, FileType Type)
      : m_Memory(MyMemory), 
      m_pBase(static_cast<PBYTE>(Address)), 
      m_Type(Type)
    { }

    // Get memory manager
    MemoryMgr PeFile::GetMemoryMgr() const
    {
      return m_Memory;
    }

    // Get base address
    PVOID PeFile::GetBase() const
    {
      return m_pBase;
    }

    // Convert RVA to VA
    PVOID PeFile::RvaToVa(DWORD Rva) const
    {
      // Convert as if data file if requested
      if (m_Type == FileType_Data)
      {
        // Ensure RVA is valid
        if (!Rva)
        {
          return nullptr;
        }

        // Get DOS header
        IMAGE_DOS_HEADER const DosHeader = m_Memory.Read<IMAGE_DOS_HEADER>(
          m_pBase);
        if (DosHeader.e_magic != IMAGE_DOS_SIGNATURE)
        {
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("PeFile::RvaToVa") << 
            ErrorString("Invalid DOS header."));
        }

        // Get NT headers
        PBYTE const pNtHeaders = m_pBase + DosHeader.e_lfanew;
        IMAGE_NT_HEADERS const NtHeadersRaw = m_Memory.Read<IMAGE_NT_HEADERS>(
          pNtHeaders);
        if (NtHeadersRaw.Signature != IMAGE_NT_SIGNATURE)
        {
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("PeFile::RvaToVa") << 
            ErrorString("Invalid NT headers."));
        }

        // Get first section header
        PIMAGE_SECTION_HEADER pSectionHeader = 
          reinterpret_cast<PIMAGE_SECTION_HEADER>(pNtHeaders + FIELD_OFFSET(
          IMAGE_NT_HEADERS, OptionalHeader) + NtHeadersRaw.FileHeader.
          SizeOfOptionalHeader);
        IMAGE_SECTION_HEADER SectionHeader = m_Memory.
          Read<IMAGE_SECTION_HEADER>(pSectionHeader);

        // Get number of sections
        WORD NumSections = NtHeadersRaw.FileHeader.NumberOfSections;

        // Loop over all sections
        for (WORD i = 0; i < NumSections; ++i)
        {
          // If RVA is in target file/raw data region perform adjustments to 
          // turn it into a VA.
          if (SectionHeader.VirtualAddress <= Rva && (SectionHeader.
            VirtualAddress + SectionHeader.Misc.VirtualSize) > Rva)
          {
            Rva -= SectionHeader.VirtualAddress;
            Rva += SectionHeader.PointerToRawData;

            return m_pBase + Rva;
          }

          // Get next section
          SectionHeader = m_Memory.Read<IMAGE_SECTION_HEADER>(
            ++pSectionHeader);
        }

        // Conversion failed
        return nullptr;
      }
      // Convert as if 'fixed' image file
      else if (m_Type == FileType_Image)
      {
        return Rva ? (m_pBase + Rva) : nullptr;
      }
      else
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("PeFile::RvaToVa") << 
          ErrorString("Unhandled file type."));
      }
    }

    // Get file type
    PeFile::FileType PeFile::GetType() const
    {
      return m_Type;
    }
  }
}

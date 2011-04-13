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

// C++ Standard Library
#include <array>

// Hades
#include "PeFile.hpp"
#include "Section.hpp"
#include "NtHeaders.hpp"
#include "MemoryMgr.hpp"

namespace Hades
{
  namespace Memory
  {
    // Constructor
    Section::Section(PeFile const& MyPeFile, WORD Number)
      : m_PeFile(MyPeFile), 
      m_Memory(m_PeFile.GetMemoryMgr()), 
      m_SectionNum(Number)
    { }

    // Get name
    std::string Section::GetName() const
    {
      // Get base of section header
      PBYTE const pSecHdr = GetBase();

      // Read RVA of module name
      std::array<char, 8> const NameData(m_Memory.Read<std::array<char, 8>>(
        pSecHdr + FIELD_OFFSET(IMAGE_SECTION_HEADER, Name)));

      // Convert section name to string
      std::string Name;
      for (std::size_t i = 0; i < 8 && NameData[i]; ++i)
      {
        Name += NameData[i];
      }

      // Return section name
      return Name;
    }

    // Get virtual address
    DWORD Section::GetVirtualAddress() const
    {
      PBYTE const pSection = GetBase();
      return m_Memory.Read<DWORD>(pSection + FIELD_OFFSET(
        IMAGE_SECTION_HEADER, VirtualAddress));
    }

    // Get virtual size
    DWORD Section::GetVirtualSize() const
    {
      PBYTE const pSection = GetBase();
      return m_Memory.Read<DWORD>(pSection + FIELD_OFFSET(
        IMAGE_SECTION_HEADER, Misc.VirtualSize));
    }

    // Get size of raw data
    DWORD Section::GetSizeOfRawData() const
    {
      PBYTE const pSection = GetBase();
      return m_Memory.Read<DWORD>(pSection + FIELD_OFFSET(
        IMAGE_SECTION_HEADER, SizeOfRawData));
    }

    // Get pointer to raw data
    DWORD Section::GetPointerToRawData() const
    {
      PBYTE const pSection = GetBase();
      return m_Memory.Read<DWORD>(pSection + FIELD_OFFSET(
        IMAGE_SECTION_HEADER, PointerToRawData));
    }

    // Get pointer to relocations
    DWORD Section::GetPointerToRelocations() const
    {
      PBYTE const pSection = GetBase();
      return m_Memory.Read<DWORD>(pSection + FIELD_OFFSET(
        IMAGE_SECTION_HEADER, PointerToRelocations));
    }

    // Get pointer to line numbers
    DWORD Section::GetPointerToLinenumbers() const
    {
      PBYTE const pSection = GetBase();
      return m_Memory.Read<DWORD>(pSection + FIELD_OFFSET(
        IMAGE_SECTION_HEADER, PointerToLinenumbers));
    }

    // Get number of relocations
    WORD Section::GetNumberOfRelocations() const
    {
      PBYTE const pSection = GetBase();
      return m_Memory.Read<WORD>(pSection + FIELD_OFFSET(
        IMAGE_SECTION_HEADER, NumberOfRelocations));
    }

    // Get number of line numbers
    WORD Section::GetNumberOfLinenumbers() const
    {
      PBYTE const pSection = GetBase();
      return m_Memory.Read<WORD>(pSection + FIELD_OFFSET(
        IMAGE_SECTION_HEADER, NumberOfLinenumbers));
    }

    // Get characteristics
    DWORD Section::GetCharacteristics() const
    {
      PBYTE const pSection = GetBase();
      return m_Memory.Read<DWORD>(pSection + FIELD_OFFSET(
        IMAGE_SECTION_HEADER, Characteristics));
    }

    // Get section header base
    PBYTE Section::GetBase() const
    {
      // Get NT headers
      NtHeaders const MyNtHeaders(m_PeFile);

      // Ensure section number is valid
      if (m_SectionNum >= MyNtHeaders.GetNumberOfSections())
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Section::GetBase") << 
          ErrorString("Invalid section number."));
      }

      // Get raw NT headers
      IMAGE_NT_HEADERS const NtHeadersRaw = MyNtHeaders.GetHeadersRaw();

      // Get pointer to raw NT headers
      PBYTE const pNtHeaders = MyNtHeaders.GetBase();

      // Get pointer to first section
      PIMAGE_SECTION_HEADER pSectionHeader = 
        reinterpret_cast<PIMAGE_SECTION_HEADER>(pNtHeaders + FIELD_OFFSET(
        IMAGE_NT_HEADERS, OptionalHeader) + NtHeadersRaw.FileHeader.
        SizeOfOptionalHeader);

      // Adjust pointer to target section
      pSectionHeader += m_SectionNum;

      // Return base of section header
      PVOID const pTempSectionHeader = pSectionHeader;
      return static_cast<PBYTE>(pTempSectionHeader);
    }

    // Get raw section header
    IMAGE_SECTION_HEADER Section::GetSectionHeaderRaw() const
    {
      // Get target raw section header
      return m_Memory.Read<IMAGE_SECTION_HEADER>(GetBase());
    }
  }
}

// Copyright Joshua Boyce 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

// Hades
#include <HadesMemory/PeLib/PeFile.hpp>
#include <HadesMemory/MemoryMgr.hpp>

namespace HadesMem
{
  // Constructor
  PeFile::PeFile(MemoryMgr const& MyMemory, PVOID Address, FileType Type)
    : m_Memory(MyMemory), 
    m_pBase(static_cast<PBYTE>(Address)), 
    m_Type(Type)
  { }
      
  // Copy constructor
  PeFile::PeFile(PeFile const& Other)
    : m_Memory(Other.m_Memory), 
    m_pBase(Other.m_pBase), 
    m_Type(Other.m_Type)
  { }
  
  // Copy assignment operator
  PeFile& PeFile::operator=(PeFile const& Other)
  {
    this->m_Memory = Other.m_Memory;
    this->m_pBase = Other.m_pBase;
    this->m_Type = Other.m_Type;
    
    return *this;
  }
  
  // Move constructor
  PeFile::PeFile(PeFile&& Other)
    : m_Memory(std::move(Other.m_Memory)), 
    m_pBase(Other.m_pBase), 
    m_Type(Other.m_Type)
  {
    Other.m_pBase = nullptr;
  }
  
  // Move assignment operator
  PeFile& PeFile::operator=(PeFile&& Other)
  {
    this->m_Memory = std::move(Other.m_Memory);
    
    this->m_pBase = Other.m_pBase;
    Other.m_pBase = nullptr;
    
    this->m_Type = Other.m_Type;
    
    return *this;
  }
  
  // Destructor
  PeFile::~PeFile()
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
  
  // Equality operator
  bool PeFile::operator==(PeFile const& Rhs) const
  {
    return m_pBase == Rhs.m_pBase && m_Memory == Rhs.m_Memory;
  }
  
  // Inequality operator
  bool PeFile::operator!=(PeFile const& Rhs) const
  {
    return !(*this == Rhs);
  }
}

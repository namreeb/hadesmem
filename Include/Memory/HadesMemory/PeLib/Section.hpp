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
#include <HadesMemory/Fwd.hpp>
#include <HadesMemory/Error.hpp>
#include <HadesMemory/MemoryMgr.hpp>
#include <HadesMemory/PeLib/PeFile.hpp>
#include <HadesMemory/PeLib/NtHeaders.hpp>

// C++ Standard Library
#include <string>
#include <iterator>

// Boost
#include <boost/optional.hpp>

// Windows
#include <Windows.h>

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
      PVOID GetBase() const;

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
      
      // Section base
      mutable PBYTE m_pBase;
    };
    
    // Section enumeration class
    class SectionList
    {
    public:
      // Section list error class
      class Error : public virtual HadesMemError
      { };
        
      // Section iterator
      class SectionIter : public boost::iterator_facade<SectionIter, Section, 
        boost::forward_traversal_tag>
      {
      public:
        // Section iterator error class
        class Error : public virtual HadesMemError
        { };

        // Constructor
        SectionIter() 
          : m_pParent(nullptr), 
          m_PeFile(), 
          m_Number(static_cast<WORD>(-1)), 
          m_Section()
        { }
        
        // Constructor
        SectionIter(SectionList& Parent) 
          : m_pParent(&Parent), 
          m_PeFile(Parent.m_PeFile), 
          m_Number(0), 
          m_Section()
        {
          if (m_Number >= NtHeaders(*m_PeFile).GetNumberOfSections())
          {
            m_pParent = nullptr;
            m_PeFile = boost::optional<PeFile>();
            m_Number = static_cast<WORD>(-1);
            m_Section = boost::optional<Section>();
          }
          else
          {
            m_Section = Section(*m_PeFile, m_Number);
          }
        }
        
        // Copy constructor
        SectionIter(SectionIter const& Rhs) 
          : m_pParent(Rhs.m_pParent), 
          m_PeFile(Rhs.m_PeFile), 
          m_Number(Rhs.m_Number), 
          m_Section(Rhs.m_Section)
        { }
        
        // Assignment operator
        SectionIter& operator=(SectionIter const& Rhs) 
        {
          m_pParent = Rhs.m_pParent;
          m_PeFile = Rhs.m_PeFile;
          m_Number = Rhs.m_Number;
          m_Section = Rhs.m_Section;
          return *this;
        }

      private:
        // Give Boost.Iterator access to internals
        friend class boost::iterator_core_access;

        // Increment iterator
        void increment() 
        {
          if (++m_Number >= NtHeaders(*m_PeFile).GetNumberOfSections())
          {
            m_pParent = nullptr;
            m_PeFile = boost::optional<PeFile>();
            m_Number = static_cast<WORD>(-1);
            m_Section = boost::optional<Section>();
          }
          else
          {
            m_Section = Section(*m_PeFile, m_Number);
          }
        }
        
        // Check iterator for equality
        bool equal(SectionIter const& Rhs) const
        {
          return this->m_pParent == Rhs.m_pParent && 
            this->m_Number == Rhs.m_Number;
        }
    
        // Dereference iterator
        Section& dereference() const 
        {
          return *m_Section;
        }

        // Parent
        class SectionList* m_pParent;
        // PE file
        boost::optional<PeFile> m_PeFile;
        // Section number
        WORD m_Number;
        // Section object
        mutable boost::optional<Section> m_Section;
      };
      
      // Section list iterator types
      typedef SectionIter iterator;
      
      // Constructor
      SectionList(PeFile const& MyPeFile)
        : m_PeFile(MyPeFile)
      { }
      
      // Get start of section list
      iterator begin()
      {
        return iterator(*this);
      }
      
      // Get end of section list
      iterator end()
      {
        return iterator();
      }
      
    private:
      // Give iterator access to internals
      friend class ExportIter;
      
      // PE file
      PeFile m_PeFile;
    };
  }
}

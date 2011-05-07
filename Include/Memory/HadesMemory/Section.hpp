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
#include <iterator>

// Boost
#include <boost/optional.hpp>

// Windows
#include <Windows.h>

// Hades
#include "Fwd.hpp"
#include "Error.hpp"
#include "PeFile.hpp"
#include "NtHeaders.hpp"
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
    
    // Section enumeration class
    class SectionList
    {
    public:
      // Section list error class
      class Error : public virtual HadesMemError
      { };
        
      // Section iterator
      template <typename SectionT>
      class SectionIter : public std::iterator<std::input_iterator_tag, 
        SectionT>
      {
      public:
        // Section iterator error class
        class Error : public virtual HadesMemError
        { };

        // Constructor
        SectionIter() 
          : m_PeFile(), 
          m_Number(static_cast<WORD>(-1)), 
          m_Section()
        { }
        
        // Constructor
        SectionIter(PeFile const& MyPeFile) 
          : m_PeFile(MyPeFile), 
          m_Number(0), 
          m_Section()
        {
          if (m_Number >= NtHeaders(*m_PeFile).GetNumberOfSections())
          {
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
        template <typename OtherT>
        SectionIter(SectionIter<OtherT> const& Rhs) 
          : m_PeFile(Rhs.m_PeFile), 
          m_Number(Rhs.Number), 
          m_Section(Rhs.m_Section)
        { }
        
        // Assignment operator
        template <typename OtherT>
        SectionIter& operator=(SectionIter<OtherT> const& Rhs) 
        {
          m_PeFile = Rhs.m_PeFile;
          m_Number = Rhs.m_Number;
          m_Section = Rhs.m_Section;
        }
        
        // Prefix increment
        SectionIter& operator++()
        {
          if (++m_Number >= NtHeaders(*m_PeFile).GetNumberOfSections())
          {
            m_PeFile = boost::optional<PeFile>();
            m_Number = static_cast<WORD>(-1);
            m_Section = boost::optional<Section>();
          }
          else
          {
            m_Section = Section(*m_PeFile, m_Number);
          }
          return *this;
        }
        
        // Postfix increment
        SectionIter operator++(int)
        {
          SectionIter Temp(*this);
          ++*this;
          return Temp;
        }
        
        // Dereference operator
        reference operator*()
        {
          return *m_Section;
        }
        
        // Dereference operator
        pointer operator->()
        {
          return &*m_Section;
        }
        
        // Equality operator
        template<typename T>
        friend bool operator==(const SectionIter<T>& Rhs, 
          const SectionIter<T>& Lhs);
        
        // Inequality operator
        template<typename T>
        friend bool operator!=(const SectionIter<T>& Rhs, 
          const SectionIter<T>& Lhs);

      private:
        // PE file
        boost::optional<PeFile> m_PeFile;
        // Section number
        WORD m_Number;
        // Section object
        boost::optional<Section> m_Section;
      };
      
      // Section list iterator types
      typedef SectionIter<const Section> const_iterator;
      typedef SectionIter<Section> iterator;
      
      // Constructor
      SectionList(PeFile const& MyPeFile)
        : m_PeFile(MyPeFile)
      { }
      
      // Get start of section list
      iterator begin()
      {
        return iterator(m_PeFile);
      }
      
      // Get end of section list
      iterator end()
      {
        return iterator();
      }
      
    private:
      // PE file
      PeFile m_PeFile;
    };
    
    // Equality operator
    template<typename T>
    inline bool operator==(SectionList::SectionIter<T> const& Lhs, 
      SectionList::SectionIter<T> const& Rhs)
    {
      return (Lhs.m_Number == Rhs.m_Number);
    }
        
    // Inequality operator
    template<typename T>    
    inline bool operator!=(SectionList::SectionIter<T> const& Lhs, 
      SectionList::SectionIter<T> const& Rhs)
    {
      return !(Lhs == Rhs);
    }
  }
}

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
#include <HadesMemory/PeLib/PeFile.hpp>
#include <HadesMemory/PeLib/NtHeaders.hpp>

// C++ Standard Library
#include <string>
#include <iterator>

// Boost
#include <boost/optional.hpp>
#include <boost/iterator/iterator_facade.hpp>

// Windows
#include <Windows.h>

namespace HadesMem
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
      
    // Copy constructor
    Section(Section const& Other);
    
    // Copy assignment operator
    Section& operator=(Section const& Other);
    
    // Move constructor
    Section(Section&& Other);
    
    // Move assignment operator
    Section& operator=(Section&& Other);
    
    // Destructor
    ~Section();

    // Get section header base
    PVOID GetBase() const;
    
    // Get section number
    WORD GetNumber() const;

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

    // Set name
    void SetName(std::string const& Name) const;

    // Set virtual address
    void SetVirtualAddress(DWORD VirtualAddress) const;

    // Set virtual size
    void SetVirtualSize(DWORD VirtualSize) const;

    // Set size of raw data
    void SetSizeOfRawData(DWORD SizeOfRawData) const;

    // Set pointer to raw data
    void SetPointerToRawData(DWORD PointerToRawData) const;

    // Set pointer to relocations
    void SetPointerToRelocations(DWORD PointerToRelocations) const;

    // Set pointer to line numbers
    void SetPointerToLinenumbers(DWORD PointerToLinenumbers) const;

    // Set number of relocations
    void SetNumberOfRelocations(WORD NumberOfRelocations) const;

    // Set number of line numbers
    void SetNumberOfLinenumbers(WORD NumberOfLinenumbers) const;

    // Set characteristics
    void SetCharacteristics(DWORD Characteristics) const;
    
    // Equality operator
    bool operator==(Section const& Rhs) const;
    
    // Inequality operator
    bool operator!=(Section const& Rhs) const;

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
    
  // Forward declaration of SectionIter
  template <typename SectionT>
  class SectionIter;
  
  // Section enumeration class
  class SectionList
  {
  public:
    // SectionList exception type
    class Error : public virtual HadesMemError
    { };
    
    // Section list iterator types
    typedef SectionIter<Section> iterator;
    typedef SectionIter<Section const> const_iterator;
    
    // Constructor
    explicit SectionList(PeFile const& MyPeFile);
    
    // Move constructor
    SectionList(SectionList&& Other);
    
    // Move assignment operator
    SectionList& operator=(SectionList&& Other);
    
    // Get start of import dir list
    iterator begin();
    
    // Get end of import dir list
    iterator end();
    
    // Get start of import dir list
    const_iterator begin() const;
     
    // Get end of import dir list
    const_iterator end() const;
    
    // Get start of import dir list
    const_iterator cbegin() const;
     
    // Get end of import dir list
    const_iterator cend() const;
    
  protected:
    // Disable copying and copy-assignment
    SectionList(SectionList const& Other);
    SectionList& operator=(SectionList const& Other);
    
  private:
    // Give SectionIter access to internals
    template <typename> friend class SectionIter;
    
    // Get section from cache by number
    boost::optional<Section&> GetByNum(DWORD Num) const;
    
    // PeFile instance
    PeFile m_PeFile;
    
    // Section cache
    mutable std::vector<Section> m_Cache;
  };

  // Section iterator
  template <typename SectionT>
  class SectionIter : public boost::iterator_facade<
    SectionIter<SectionT>, SectionT, boost::forward_traversal_tag>
  {
  public:
    // SectionIter error class
    class Error : public virtual HadesMemError
    { };

    // Constructor
    SectionIter()
      : m_pParent(nullptr), 
      m_Number(static_cast<DWORD>(-1)), 
      m_Current()
    { }
    
    // Constructor
    SectionIter(class SectionList const& Parent)
      : m_pParent(&Parent), 
      m_Number(0), 
      m_Current()
    {
      boost::optional<Section&> Temp = m_pParent->GetByNum(m_Number);
      if (Temp)
      {
        m_Current = *Temp;
      }
      else
      {
        m_pParent = nullptr;
        m_Number = static_cast<DWORD>(-1);
      }
    }
    
  private:
    // Give Boost.Iterator access to internals
    friend class boost::iterator_core_access;

    // Increment iterator
    void increment()
    {
      boost::optional<Section&> Temp = m_pParent->GetByNum(++m_Number);
      m_Current = Temp ? *Temp : boost::optional<Section>();
      if (!Temp)
      {
        m_pParent = nullptr;
        m_Number = static_cast<DWORD>(-1);
      }
    }
    
    // Check iterator for equality
    bool equal(SectionIter const& Rhs) const
    {
      return this->m_pParent == Rhs.m_pParent && 
        this->m_Number == Rhs.m_Number;
    }

    // Dereference iterator
    SectionT& dereference() const
    {
      return *m_Current;
    }

    // Parent list instance
    class SectionList const* m_pParent;
    
    // Import dir number
    DWORD m_Number;
    
    // Current import dir instance
    mutable boost::optional<Section> m_Current;
  };
}

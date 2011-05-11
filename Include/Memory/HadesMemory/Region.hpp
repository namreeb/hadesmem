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
#include <memory>

// Boost
#include <boost/optional.hpp>
#include <boost/filesystem.hpp>
#include <boost/iterator/iterator_facade.hpp>

// Windows API
#include <Windows.h>

// Hades
#include "Fwd.hpp"
#include "Error.hpp"
#include "MemoryMgr.hpp"

namespace Hades
{
  namespace Memory
  {
    // Memory region managing class
    class Region
    {
    public:
      // MemRegion exception type
      class Error : public virtual HadesMemError 
      { };

      // Constructor
      Region(MemoryMgr const& MyMemory, PVOID Address);

      // Constructor
      Region(MemoryMgr const& MyMemory, MEMORY_BASIC_INFORMATION const& MyMbi);

      // Get base address
      PVOID GetBase() const;
      
      // Get allocation base
      PVOID GetAllocBase() const;
      
      // Get allocation protection
      DWORD GetAllocProtect() const;
      
      // Get size
      SIZE_T GetSize() const;
      
      // Get state
      DWORD GetState() const;
      
      // Get protection
      DWORD GetProtect() const;
      
      // Get type
      DWORD GetType() const;
      
      // Dump to file
      void Dump(boost::filesystem::path const& Path) const;

    private:
      // MemoryMgr instance
      MemoryMgr m_Memory;

      // Region information
      MEMORY_BASIC_INFORMATION m_RegionInfo;
    };
    
    // Region enumeration class
    class RegionList
    {
    public:
      // Region list error class
      class Error : public virtual HadesMemError
      { };
        
      // Region iterator
      template <typename RegionT>
      class RegionIter : public std::iterator<std::input_iterator_tag, 
        RegionT>
      {
      public:
        typedef typename std::iterator<std::input_iterator_tag, RegionT>::
          reference reference;
        typedef typename std::iterator<std::input_iterator_tag, RegionT>::
          pointer pointer;
        typedef typename std::iterator<std::input_iterator_tag, RegionT>::
          iterator iterator;
          
        // Region iterator error class
        class Error : public virtual HadesMemError
        { };

        // Constructor
        RegionIter() 
          : m_pParent(nullptr), 
          m_Memory(), 
          m_Address(reinterpret_cast<PVOID>(-1)), 
          m_Region(), 
          m_RegionSize(0)
        { }
        
        // Constructor
        RegionIter(RegionList& Parent, MemoryMgr const& MyMemory) 
          : m_pParent(&Parent), 
          m_Memory(MyMemory), 
          m_Address(nullptr), 
          m_Region(), 
          m_RegionSize()
        {
          // Get region info
          MEMORY_BASIC_INFORMATION MyMbi = { 0 };
          if (VirtualQueryEx(m_Memory->GetProcessHandle(), m_Address, &MyMbi, 
            sizeof(MyMbi)))
          {
            m_Address = MyMbi.BaseAddress;
            m_RegionSize = MyMbi.RegionSize;
  
            m_Region = Region(*m_Memory, MyMbi);
          }
          else
          {
            m_pParent = nullptr;
            m_Memory = boost::optional<MemoryMgr>();
            m_Address = reinterpret_cast<PVOID>(-1);
            m_Region = boost::optional<Region>();
            m_RegionSize = 0;
          }
        }
        
        // Copy constructor
        template <typename OtherT>
        RegionIter(RegionIter<OtherT> const& Rhs) 
          : m_pParent(Rhs.m_pParent), 
          m_Memory(Rhs.m_Memory), 
          m_Address(Rhs.m_Address), 
          m_Region(Rhs.m_Region), 
          m_RegionSize(Rhs.m_RegionSize)
        { }
        
        // Assignment operator
        template <typename OtherT>
        RegionIter& operator=(RegionIter<OtherT> const& Rhs) 
        {
          m_pParent = Rhs.m_pParent;
          m_Memory = Rhs.m_Memory;
          m_Address = Rhs.m_Address;
          m_Region = Rhs.m_Region;
          m_RegionSize = Rhs.m_RegionSize;
        }
        
        // Prefix increment
        RegionIter& operator++()
        {
          // Advance to next region
          m_Address = static_cast<PBYTE>(m_Address) + m_RegionSize;
  
          // Get region info
          MEMORY_BASIC_INFORMATION MyMbi = { 0 };
          if (VirtualQueryEx(m_Memory->GetProcessHandle(), m_Address, &MyMbi, 
            sizeof(MyMbi)))
          {
            m_Address = MyMbi.BaseAddress;
            m_RegionSize = MyMbi.RegionSize;
  
            m_Region = Region(*m_Memory, MyMbi);
          }
          else
          {
            m_pParent = nullptr;
            m_Memory = boost::optional<MemoryMgr>();
            m_Address = reinterpret_cast<PVOID>(-1);
            m_Region = boost::optional<Region>();
            m_RegionSize = 0;
          }
          
          return *this;
        }
        
        // Postfix increment
        RegionIter operator++(int)
        {
          RegionIter Temp(*this);
          ++*this;
          return Temp;
        }
        
        // Dereference operator
        reference operator*()
        {
          return *m_Region;
        }
        
        // Dereference operator
        pointer operator->()
        {
          return &*m_Region;
        }
        
        // Equality operator
        template<typename T>
        friend bool operator==(const RegionIter<T>& Rhs, 
          const RegionIter<T>& Lhs);
        
        // Inequality operator
        template<typename T>
        friend bool operator!=(const RegionIter<T>& Rhs, 
          const RegionIter<T>& Lhs);

      private:
        // Parent
        class RegionList* m_pParent;
        // Memory instance
        boost::optional<MemoryMgr> m_Memory;
        // Region address
        PVOID m_Address;
        // Region object
        boost::optional<Region> m_Region;
        // Region size
        SIZE_T m_RegionSize;
      };
      
      // Region list iterator types
      typedef RegionIter<const Region> const_iterator;
      typedef RegionIter<Region> iterator;
      
      // Constructor
      RegionList(MemoryMgr const& MyMemory)
        : m_Memory(MyMemory)
      { }
      
      // Get start of Region list
      iterator begin()
      {
        return iterator(*this, m_Memory);
      }
      
      // Get end of Region list
      iterator end()
      {
        return iterator();
      }
      
    private:
      // Memory instance
      MemoryMgr m_Memory;
    };
    
    // Equality operator
    template<typename T>
    inline bool operator==(RegionList::RegionIter<T> const& Lhs, 
      RegionList::RegionIter<T> const& Rhs)
    {
      return (Lhs.m_pParent == Rhs.m_pParent && 
        Lhs.m_Address == Rhs.m_Address);
    }
        
    // Inequality operator
    template<typename T>    
    inline bool operator!=(RegionList::RegionIter<T> const& Lhs, 
      RegionList::RegionIter<T> const& Rhs)
    {
      return !(Lhs == Rhs);
    }
  }
}

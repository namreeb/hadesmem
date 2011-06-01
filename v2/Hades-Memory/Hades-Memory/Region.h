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

// Windows API
#include <Windows.h>

// C++ Standard Library
#include <memory>

// Boost
#pragma warning(push, 1)
#pragma warning (disable: ALL_CODE_ANALYSIS_WARNINGS)
#include <boost/optional.hpp>
#include <boost/iterator/iterator_facade.hpp>
#pragma warning(pop)

// Hades
#include "Fwd.h"
#include "Error.h"
#include "MemoryMgr.h"

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

    private:
      // MemoryMgr instance
      MemoryMgr m_Memory;

      // Region information
      MEMORY_BASIC_INFORMATION m_RegionInfo;
    };

    // Region iterator
    class RegionListIter : public boost::iterator_facade<RegionListIter, 
      boost::optional<Region>, boost::incrementable_traversal_tag>
    {
    public:
      // Constructor
      RegionListIter(MemoryMgr const& MyMemory) 
        : m_Memory(MyMemory), 
        m_BaseAddress(nullptr), 
        m_RegionSize(0), 
        m_Current()
      {
        MEMORY_BASIC_INFORMATION MyMbi = { 0 };
        if (!VirtualQueryEx(m_Memory.GetProcessHandle(), m_BaseAddress, &MyMbi, 
          sizeof(MyMbi)))
        {
          DWORD const LastError = GetLastError();
          BOOST_THROW_EXCEPTION(Region::Error() << 
            ErrorFunction("RegionEnum::First") << 
            ErrorString("Could not get first memory region.") << 
            ErrorCodeWin(LastError));
        }

        m_BaseAddress = MyMbi.BaseAddress;
        m_RegionSize = MyMbi.RegionSize;

        m_Current = Region(m_Memory, MyMbi);
      }

    private:
      // Allow Boost.Iterator access to internals
      friend class boost::iterator_core_access;

      // For Boost.Iterator
      void increment() 
      {
        // Advance to next region
        m_BaseAddress = static_cast<PBYTE>(m_BaseAddress) + m_RegionSize;

        // Get region info
        // Fixme: Check GetLastError to ensure EOL and throw an exception 
        // on an actual error.
        MEMORY_BASIC_INFORMATION MyMbi = { 0 };
        if (VirtualQueryEx(m_Memory.GetProcessHandle(), m_BaseAddress, &MyMbi, 
          sizeof(MyMbi)))
        {
          m_BaseAddress = MyMbi.BaseAddress;
          m_RegionSize = MyMbi.RegionSize;

          m_Current = Region(m_Memory, MyMbi);
        }
        else
        {
          m_Current = boost::optional<Region>();
        }
      }

      // For Boost.Iterator
      boost::optional<Region>& dereference() const
      {
        return m_Current;
      }

      // Memory instance
      MemoryMgr m_Memory;

      // Current address
      PVOID m_BaseAddress;

      // Current region size
      SIZE_T m_RegionSize;

      // Current region
      mutable boost::optional<Region> m_Current;
    };

  }
}

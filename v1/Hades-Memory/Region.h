/*
This file is part of HadesMem.
Copyright © 2010 Cypherjb (aka Chazwazza, aka Cypher). 
<http://www.cypherjb.com/> <cypher.jb@gmail.com>

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
#include <vector>
#include <iostream>

// Boost
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

// HadesMem
#include "Memory.h"

namespace Hades
{
  namespace Memory
  {
    // MemRegion exception type
    class RegionError : public virtual HadesMemError 
    { };

    // Get memory region list
    inline std::vector<boost::shared_ptr<class Region>> GetMemoryRegionList(
      MemoryMgr const& MyMemory);

    // Region wide stream overload
    inline std::wostream& operator<< (std::wostream& Out, 
      class Region const& In);

    // Memory region managing class
    class Region
    {
    public:
      // Constructor
      inline explicit Region(MemoryMgr const& MyMemory, PVOID Address);

      // Get base address
      inline PVOID GetBase() const;
      // Get allocation base
      inline PVOID GetAllocBase() const;
      // Get allocation protection
      inline DWORD GetAllocProtect() const;
      // Get size
      inline SIZE_T GetSize() const;
      // Get state
      inline DWORD GetState() const;
      // Get protection
      inline DWORD GetProtect() const;
      // Get type
      inline DWORD GetType() const;

    private:
      // Disable assignment
      Region& operator= (Region const&);

      // MemoryMgr instance
      MemoryMgr const& m_Memory;

      // Region information
      MEMORY_BASIC_INFORMATION m_RegionInfo;
    };

    // Get memory region list
    std::vector<boost::shared_ptr<Region>> GetMemoryRegionList(
      MemoryMgr const& MyMemory)
    {
      // Region list
      std::vector<boost::shared_ptr<Region>> RegionList;

      // Loop over all memory regions
      PBYTE Address = nullptr;
      MEMORY_BASIC_INFORMATION MyMbi = { 0 };
      while (VirtualQueryEx(MyMemory.GetProcessHandle(), Address, &MyMbi, 
        sizeof(MyMbi)))
      {
        // Add current region to list
        RegionList.push_back(boost::make_shared<Region>(MyMemory, 
          MyMbi.BaseAddress));
        // Advance to next region
        Address = reinterpret_cast<PBYTE>(MyMbi.BaseAddress) + 
          MyMbi.RegionSize;
      }

      // Return region list
      return RegionList;
    }

    // Region wide stream overload
    std::wostream& operator<< (std::wostream& Out, Region const& In)
    {
      Out << "Base Address: " << In.GetBase() << "." << std::endl;
      Out << "Allocation Base: " << In.GetAllocBase() << "." << std::endl;
      Out << "Allocation Protect: " << In.GetAllocProtect() << "." << 
        std::endl;
      Out << "Region Size: " << In.GetSize() << "." << std::endl;
      Out << "State: " << In.GetState() << "." << std::endl;
      Out << "Protect: " << In.GetProtect() << "." << std::endl;
      Out << "Type: " << In.GetType() << "." << std::endl;
      return Out;
    }

    // Constructor
    Region::Region(MemoryMgr const& MyMemory, PVOID Address) 
      : m_Memory(MyMemory), 
      m_RegionInfo() 
    {
      // Clear region info
      ZeroMemory(&m_RegionInfo, sizeof(m_RegionInfo));

      // Query region info
      if (!VirtualQueryEx(m_Memory.GetProcessHandle(), Address, 
        &m_RegionInfo, sizeof(m_RegionInfo)))
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(RegionError() << 
          ErrorFunction("Region::Region") << 
          ErrorString("Could not query memory region.") << 
          ErrorCodeWin(LastError));
      }
    }

    // Get base address
    PVOID Region::GetBase() const
    {
      return m_RegionInfo.BaseAddress;
    }

    // Get allocation base
    PVOID Region::GetAllocBase() const
    {
      return m_RegionInfo.AllocationBase;
    }

    // Get allocation protection
    DWORD Region::GetAllocProtect() const
    {
      return m_RegionInfo.AllocationProtect;
    }

    // Get size
    SIZE_T Region::GetSize() const
    {
      return m_RegionInfo.RegionSize;
    }

    // Get state
    DWORD Region::GetState() const
    {
      return m_RegionInfo.State;
    }

    // Get protection
    DWORD Region::GetProtect() const
    {
      return m_RegionInfo.Protect;
    }

    // Get type
    DWORD Region::GetType() const
    {
      return m_RegionInfo.Type;
    }
  }
}

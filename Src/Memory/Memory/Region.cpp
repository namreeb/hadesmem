/*
This file is part of HadesMem.
Copyright (C) 2010 Joshua Boyce (aka RaptorFactor, Cypherjb, Cypher, Chazwazza).
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
#include "Region.hpp"

// Boost
#pragma warning(push, 1)
#include <boost/filesystem/fstream.hpp>
#pragma warning(pop)

namespace Hades
{
  namespace Memory
  {
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
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Region::Region") << 
          ErrorString("Could not query memory region.") << 
          ErrorCode(LastError));
      }
    }

    // Constructor
    Region::Region(MemoryMgr const& MyMemory, 
      MEMORY_BASIC_INFORMATION const& MyMbi) 
      : m_Memory(MyMemory), 
      m_RegionInfo(MyMbi)
    { }

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
    
    // Dump to file
    void Region::Dump(boost::filesystem::path const& Path) const
    {
      boost::filesystem::ofstream Out(Path, std::ios::binary | std::ios::trunc);
      if (!Out)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Region::Dump") << 
          ErrorString("Could not open dump file."));
      }
      
      auto RegionBuf(m_Memory.Read<std::vector<BYTE>>(GetBase(), GetSize()));
      Out.write(reinterpret_cast<char const*>(&RegionBuf[0]), 
        RegionBuf.size());
      if (!Out)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Region::Dump") << 
          ErrorString("Error writing to dump file."));
      }
    }
  }
}

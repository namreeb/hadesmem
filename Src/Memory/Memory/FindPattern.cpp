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
#include <HadesMemory/FindPattern.hpp>
#include <HadesCommon/Config.hpp>
#include <HadesMemory/Module.hpp>
#include <HadesMemory/PeLib/PeFile.hpp>
#include <HadesMemory/PeLib/DosHeader.hpp>
#include <HadesMemory/PeLib/NtHeaders.hpp>

// C++ Standard Library
#include <limits>
#include <vector>
#include <string>
#include <algorithm>

// Boost
#include <boost/assert.hpp>

namespace Hades
{
  namespace Memory
  {
    // Constructor
    FindPattern::FindPattern(MemoryMgr const& MyMemory, HMODULE Module) 
      : m_Memory(MyMemory), 
      m_Base(0), 
      m_Start(nullptr), 
      m_End(nullptr), 
      m_Addresses()
    {
      // Get base of exe if required
      if (!Module)
      {
        ModuleList Modules(m_Memory);
        Module = Modules.begin()->GetBase();
      }
      
      // Ensure file is a valid PE file
      PBYTE const pBase = reinterpret_cast<PBYTE>(Module);
      m_Base = reinterpret_cast<DWORD_PTR>(pBase);
      BOOST_ASSERT(m_Base != 0);
      PeFile MyPeFile(m_Memory, pBase);
      DosHeader const MyDosHeader(MyPeFile);
      NtHeaders const MyNtHeaders(MyPeFile);

      // Get base of code section
      m_Start = pBase + MyNtHeaders.GetBaseOfCode();
      BOOST_ASSERT(m_Start != nullptr);

      // Calculate end of code section
      m_End = m_Start + MyNtHeaders.GetSizeOfCode();
      BOOST_ASSERT(m_End != nullptr);
    }
    
    // Find pattern
    PVOID FindPattern::Find(std::wstring const& Data, 
      std::wstring const& Mask, FindFlags Flags) const
    {
      // Ensure pattern attributes are valid
      if (Data.empty() || Mask.empty())
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("FindPattern::Find") << 
          ErrorString("Empty pattern or mask data."));
      }
      
      // Sanity check
      if (Data.size() != Mask.size() * 2 + (Mask.size() - 1))
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("FindPattern::Find") << 
          ErrorString("Invalid mask or data size."));
      }

      // Convert data to byte buffer
      std::vector<std::pair<BYTE, bool>> DataBuf;
      for (std::size_t i = 0; i != Mask.size(); ++i)
      {
        bool MaskFlag = (Mask[i] == L'x');
        int Current = 0;
        if (MaskFlag)
        {
          try
          {
            Current = std::stoi(Data.substr(i * 3, ((i * 3) + 2)), nullptr, 
              16);
          }
          catch (std::exception const& /*e*/)
          {
            BOOST_THROW_EXCEPTION(Error() << 
              ErrorFunction("FindPattern::Find") << 
              ErrorString("Invalid data conversion."));
          }
        }
        
        BOOST_ASSERT(Current >= (std::numeric_limits<BYTE>::min)() && 
          Current <= (std::numeric_limits<BYTE>::max)());

        DataBuf.push_back(std::make_pair(static_cast<BYTE>(Current), 
          MaskFlag));
      }

      // Search memory for pattern
      PVOID Address = Find(DataBuf);
      if (!Address && ((Flags & ThrowOnUnmatch) == ThrowOnUnmatch))
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("FindPattern::Find") << 
          ErrorString("Could not match pattern."));
      }
      
      // Convert to relative address if required
      if (Address && ((Flags & RelativeAddress) == RelativeAddress))
      {
        Address = static_cast<PBYTE>(Address) - m_Base;
      }
      
      // Return matched address
      return Address;
    }

    // Find pattern
    PVOID FindPattern::Find(std::wstring const& Data, 
      std::wstring const& Mask, std::wstring const& Name, 
      FindFlags Flags)
    {
      // Search memory for pattern
      PVOID Address = Find(Data, Mask, Flags);
      
      // Store address if name is specified
      if (!Name.empty())
      {
        m_Addresses[Name] = Address;
      }
      
      // Return pointer
      return Address;
    }

    // Search memory
    PVOID FindPattern::Find(std::vector<std::pair<BYTE, bool>> const& Data) 
      const
    {
      // Cache all memory to be scanned
      BOOST_ASSERT(m_End > m_Start);
      std::size_t MemSize = m_End - m_Start;
      std::vector<BYTE> Buffer(m_Memory.Read<std::vector<BYTE>>(m_Start, 
        MemSize));

      // Scan memory
      auto Iter = std::search(Buffer.cbegin(), Buffer.cend(), Data.cbegin(), 
        Data.cend(), 
        [] (BYTE HCur, std::pair<BYTE, bool> const& NCur)
        {
          return (!NCur.second) || (HCur == NCur.first);
        });

      // Return address if found or null if not found
      return 
        (Iter != Buffer.cend()) 
        ? (m_Start + std::distance(Buffer.cbegin(), Iter)) 
        : nullptr;
    }

    // Get address map
    std::map<std::wstring, PVOID> FindPattern::GetAddresses() const
    {
      return m_Addresses;
    }

    // Operator[] overload to allow retrieving addresses by name
    PVOID FindPattern::operator[](std::wstring const& Name) const
    {
      auto const Iter = m_Addresses.find(Name);
      return Iter != m_Addresses.end() ? Iter->second : nullptr;
    }
  }
}

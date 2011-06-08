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
#include <HadesCommon/I18n.hpp>
#include <HadesCommon/Config.hpp>
#include <HadesMemory/Module.hpp>
#include <HadesMemory/PeLib/PeFile.hpp>
#include <HadesMemory/PeLib/Section.hpp>
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
      m_CodeRegions(), 
      m_DataRegions(), 
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

      // Get scan regions (sections marked as code and/or data)
      SectionList Sections(MyPeFile);
      std::for_each(Sections.begin(), Sections.end(), 
        [&] (Section const& S)
        {
          if ((S.GetCharacteristics() & IMAGE_SCN_CNT_CODE) == 
            IMAGE_SCN_CNT_CODE)
          {
            PBYTE SBegin = static_cast<PBYTE>(MyPeFile.RvaToVa(
              S.GetVirtualAddress()));
            BOOST_ASSERT(SBegin != nullptr);
            PBYTE SEnd = SBegin + S.GetSizeOfRawData();
            BOOST_ASSERT(SEnd > SBegin);
            m_CodeRegions.push_back(std::make_pair(SBegin, SEnd));
          }
          
          if ((S.GetCharacteristics() & IMAGE_SCN_CNT_INITIALIZED_DATA) == 
            IMAGE_SCN_CNT_INITIALIZED_DATA)
          {
            PBYTE SBegin = static_cast<PBYTE>(MyPeFile.RvaToVa(
              S.GetVirtualAddress()));
            BOOST_ASSERT(SBegin != nullptr);
            PBYTE SEnd = SBegin + S.GetSizeOfRawData();
            BOOST_ASSERT(SEnd > SBegin);
            m_DataRegions.push_back(std::make_pair(SBegin, SEnd));
          }
        });
        
      // Ensure we found at least one section to scan
      if (m_CodeRegions.empty() && m_DataRegions.empty())
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("FindPattern::FindPattern") << 
          ErrorString("No valid sections to scan found."));
      }
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
#ifndef HADES_MSVC
            std::wstringstream ss;
            if (!(ss << std::hex << Data.substr(i * 3, ((i * 3) + 2))))
            {
              throw std::exception();
            }
            if (!(ss >> Current))
            {
              throw std::exception();
            }
#else
            Current = std::stoi(Data.substr(i * 3, ((i * 3) + 2)), nullptr, 
              16);
#endif
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
      bool ScanDataSecs = ((Flags & ScanData) == ScanData);
      PVOID Address = Find(DataBuf, ScanDataSecs);
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
    PVOID FindPattern::Find(std::vector<std::pair<BYTE, bool>> const& Data, 
      bool ScanDataSecs) const
    {
      // Scan all specified section until we find something
      std::vector<std::pair<PBYTE, PBYTE>> const& ScanRegions = 
        ScanDataSecs ? m_DataRegions : m_CodeRegions;
      for (auto i = ScanRegions.cbegin(); i != ScanRegions.cend(); ++i)
      {
        // Get section start and end
        PBYTE SBegin = i->first;
        PBYTE SEnd = i->second;
        BOOST_ASSERT(SEnd > SBegin);
        
        // Cache all memory to be scanned
        std::size_t MemSize = SEnd - SBegin;
        std::vector<BYTE> Buffer(m_Memory.Read<std::vector<BYTE>>(SBegin, 
          MemSize));
  
        // Scan memory
        auto Iter = std::search(Buffer.cbegin(), Buffer.cend(), Data.cbegin(), 
          Data.cend(), 
          [] (BYTE HCur, std::pair<BYTE, bool> const& NCur)
          {
            return (!NCur.second) || (HCur == NCur.first);
          });
  
        // Return address if found
        if (Iter != Buffer.cend())
        {
          return (SBegin + std::distance(Buffer.cbegin(), Iter));
        }
      }
      
      // Nothing found
      return nullptr;
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
    
    // Constructor
    Pattern::Pattern(FindPattern& Finder, std::wstring const& Data, 
	    std::wstring const& Mask, std::wstring const& Name, 
	    FindPattern::FindFlags Flags)
	    : m_Finder(Finder), 
	    m_Name(Name), 
	    m_Address(static_cast<PBYTE>(Finder.Find(Data, Mask, Flags))), 
	    m_Flags(Flags)
	  { }
    
    // Constructor
    Pattern::Pattern(FindPattern& Finder, std::wstring const& Data, 
	    std::wstring const& Mask, FindPattern::FindFlags Flags)
	    : m_Finder(Finder), 
	    m_Name(), 
	    m_Address(static_cast<PBYTE>(Finder.Find(Data, Mask, Flags))), 
	    m_Flags(Flags)
	  { }
	  
	  // Save back to parent
	  void Pattern::Save()
	  {
	    if (m_Name.empty())
	    {
	      return;
	    }
	    
	    m_Finder.m_Addresses[m_Name] = m_Address;
	  }
		
		// Update address
	  void Pattern::Update(PBYTE Address)
	  {
	    m_Address = Address;
	  }
	  
	  // Get address
	  PBYTE Pattern::GetAddress() const
	  {
	    return m_Address;
	  }
	  
	  // Get memory manager
	  MemoryMgr Pattern::GetMemory() const
	  {
	    return m_Finder.m_Memory;
	  }
	  
	  // Get find flags
	  FindPattern::FindFlags Pattern::GetFlags() const
	  {
	    return m_Flags;
	  }
	  
	  // Get base
	  DWORD_PTR Pattern::GetBase() const
	  {
	    return m_Finder.m_Base;
	  }
	  
	  namespace PatternManipulators
	  {
	    void Manipulator::Manipulate(Pattern& /*Pat*/) const
	    { }
  
  		Pattern& operator<< (Pattern& Pat, Manipulator const& Manip)
  		{
  			Manip.Manipulate(Pat);
  			return Pat;
  		}
  		
	    void Save::Manipulate(Pattern& Pat) const
	    {
        Pat.Save();
	    }
	    
	    Add::Add(DWORD_PTR Offset)
	      : m_Offset(Offset)
	    { }
	    
	    void Add::Manipulate(Pattern& Pat) const
	    {
        PBYTE Address = Pat.GetAddress();
        if (Address)
        {
          Pat.Update(Address + m_Offset);
        }
	    }
	    
	    Sub::Sub(DWORD_PTR Offset)
	      : m_Offset(Offset)
	    { }
	    
	    void Sub::Manipulate(Pattern& Pat) const
	    {
        PBYTE Address = Pat.GetAddress();
        if (Address)
        {
          Pat.Update(Address - m_Offset);
        }
	    }
	    
	    void Lea::Manipulate(Pattern& Pat) const
      {
        PBYTE Address = Pat.GetAddress();
        if (Address)
        {
          try
          {
            DWORD_PTR Base = ((Pat.GetFlags() & FindPattern::RelativeAddress) == 
              FindPattern::RelativeAddress) ? Pat.GetBase() : 0;
            Address = Pat.GetMemory().Read<PBYTE>(Pat.GetAddress() + Base);
            Pat.Update(Address - Base);
          }
          catch (MemoryMgr::Error const& /*e*/)
          {
            Pat.Update(nullptr);
          }
        }
      }
	    
	    Rel::Rel(DWORD_PTR Size, DWORD_PTR Offset)
	      : m_Size(Size), 
	      m_Offset(Offset)
	    { }
	    
	    void Rel::Manipulate(Pattern& Pat) const
      {
        PBYTE Address = Pat.GetAddress();
        if (Address)
        {
          try
          {
            DWORD_PTR Base = ((Pat.GetFlags() & FindPattern::RelativeAddress) == 
              FindPattern::RelativeAddress) ? Pat.GetBase() : 0;
            Address = Pat.GetMemory().Read<PBYTE>(Address + Base) + 
              reinterpret_cast<DWORD_PTR>(Address + Base) + m_Size - m_Offset;
            Pat.Update(Address - Base);
          }
          catch (MemoryMgr::Error const& /*e*/)
          {
            Pat.Update(nullptr);
          }
        }
      }
	  }
  }
}

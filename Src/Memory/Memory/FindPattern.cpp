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
#include <locale>
#include <iterator>
#include <algorithm>

// Boost
#ifdef HADES_MSVC
#pragma warning(push, 1)
#endif
#include <boost/assert.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#ifdef HADES_MSVC
#pragma warning(pop)
#endif

namespace Hades
{
  namespace Memory
  {
    // Pattern info for parser
    struct PatternInfo
    {
      std::wstring Name;
      std::wstring Data;
    };
    
    // Pattern manipulator info for parser
    struct ManipInfo
    {
      std::wstring Name;
      std::vector<unsigned> Operands;
    };
    
    // Full pattern info for parser (both pattern and manipulator info)
    struct PatternInfoFull
    {
      PatternInfo Pattern;
      std::vector<ManipInfo> Manipulators;
    };
  }
}

// Adapt pattern info struct for parser
BOOST_FUSION_ADAPT_STRUCT(Hades::Memory::PatternInfo, 
  (std::wstring, Name)
  (std::wstring, Data)
  (std::wstring, Mask))

// Adapt pattern manipulator info struct for parser
BOOST_FUSION_ADAPT_STRUCT(Hades::Memory::ManipInfo, 
  (std::wstring, Name)
  (std::vector<unsigned>, Operands))

// Adapt full pattern info struct for parser
BOOST_FUSION_ADAPT_STRUCT(Hades::Memory::PatternInfoFull, 
  (Hades::Memory::PatternInfo, Pattern)
  (std::vector<Hades::Memory::ManipInfo>, Manipulators))

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
          // Handle sections marked as code
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
          
          // Handle sections marked as (initialized) data
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
    PVOID FindPattern::Find(std::wstring const& Data, FindFlags Flags) const
    {
      // Ensure pattern attributes are valid
      if (Data.size() < 2)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("FindPattern::Find") << 
          ErrorString("Pattern length invalid."));
      }
      
      // Ensure whitespace in pattern data is correct
      for (std::size_t i = 2; i < Data.size(); i += 3)
      {
        if (Data[i] != L' ')
        {
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("FindPattern::Find") << 
            ErrorString("Pattern data invalid (whitespace)."));
        }
      }
        
      // Convert data to byte buffer
      std::vector<std::pair<BYTE, bool>> DataBuf;
      for (std::size_t i = 0; i < Data.size(); i += 3)
      {
        // Get current byte as string
        std::wstring const ByteStr = Data.substr(i, 2);
        bool IsWildcard = (ByteStr == L"??");
        bool IsHex = (std::isxdigit(ByteStr[0]) && std::isxdigit(ByteStr[1]));
        if (!IsWildcard && !IsHex)
        {
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("FindPattern::Find") << 
            ErrorString("Invalid data (bytes)."));
        }
             
        // Get data for non-wildcards by converting hex string to integer
        unsigned int Current = 0;
        if (!IsWildcard)
        {
          auto ByteStrBeg = ByteStr.cbegin();
          auto ByteStrEnd = ByteStr.cend();
          bool Converted = boost::spirit::qi::parse(ByteStrBeg, 
            ByteStrEnd, boost::spirit::qi::hex, Current);
          if (!Converted || ByteStrBeg != ByteStrEnd)
          {
            BOOST_THROW_EXCEPTION(Error() << 
              ErrorFunction("FindPattern::Find") << 
              ErrorString("Invalid data conversion."));
          }
        }
        
        // The data should be in the range 0x00 - 0xFF and hence should 
        // always fit in a BYTE
        BOOST_ASSERT(Current >= (std::numeric_limits<BYTE>::min)() && 
          Current <= (std::numeric_limits<BYTE>::max)());

        // Add current data and mask to list
        DataBuf.push_back(std::make_pair(static_cast<BYTE>(Current), 
          !IsWildcard));
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
      std::wstring const& Name, FindFlags Flags)
    {
      // Search memory for pattern
      PVOID Address = Find(Data, Flags);
      
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
      
    // Read patterns from file
    void FindPattern::LoadFile(boost::filesystem::path const& Path)
    {
      // Open current file
      boost::filesystem::wifstream PatternFile(Path);
      if (!PatternFile)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("FindPattern::LoadFile") << 
          ErrorString("Could not open pattern file."));
      }

      // Copy file to buffer
      std::istreambuf_iterator<wchar_t> const PatFileBeg(PatternFile);
      std::istreambuf_iterator<wchar_t> const PatFileEnd;
      std::vector<wchar_t> PatFileBuf(PatFileBeg, PatFileEnd);
      PatFileBuf.push_back(L'\0');
      
      // Parse pattern file
      LoadFileMemory(PatFileBuf.data());
    }
    
    // Read patterns from memory
    void FindPattern::LoadFileMemory(std::wstring const& Data)
    {
      namespace qi = boost::spirit::qi;
      
      // Data iterator type (wide string)
      typedef std::wstring::const_iterator DataIter;
      // Parser skipper type (whitespace)
      typedef qi::standard::space_type SkipWsT;
        
      // Map flag names to values
      struct FlagsParserT : qi::symbols<wchar_t, FindFlags> 
      {
        FlagsParserT()
        {
          add
            (L"None", None)
            (L"ThrowOnUnmatch", ThrowOnUnmatch)
            (L"RelativeAddress", RelativeAddress)
            (L"ScanData", ScanData);
        }
      } FlagsParser;
      
      // Flags parser
      qi::rule<DataIter, std::vector<FindFlags>(), SkipWsT> FlagsRule = 
        '(' >> *(FlagsParser % ',') >> ')';
        
      // Pattern name parser
      qi::rule<DataIter, std::wstring()> NameRule = 
        qi::lexeme[*(~qi::char_(','))] >> ',';
      
      // Pattern data parser
      qi::rule<DataIter, std::wstring()> DataRule = 
        qi::lexeme[*(~qi::char_('}'))];
      
      // Pattern parser
      qi::rule<DataIter, PatternInfo(), SkipWsT> PatternRule = 
        '{' >> NameRule >> DataRule >> '}';
      
      // Manipulator name parser
      qi::rule<DataIter, std::wstring(), SkipWsT> ManipNameRule = 
        +(~qi::char_(',')) >> ',';
      
      // Manipulator operand parser
      qi::rule<DataIter, std::vector<unsigned>(), SkipWsT> OperandRule = 
        *(qi::uint_ % ',');
      
      // Manipulator parser
      qi::rule<DataIter, ManipInfo(), SkipWsT> ManipRule = 
        '[' >> ManipNameRule >> OperandRule >> ']';
      
      // Pattern parser
      qi::rule <DataIter, PatternInfoFull(), SkipWsT> PatternFullRule = 
        PatternRule >> *ManipRule;
      
      // Flag list
      std::vector<FindFlags> FlagsList;
      // Pattern list
      std::vector<PatternInfoFull> PatternList;
      
      // Parse pattern file
      auto DataBeg = Data.cbegin();
      auto DataEnd = Data.cend();
      bool Parsed = qi::phrase_parse(DataBeg, DataEnd, 
        (
          L"HadesMem Patterns" >> FlagsRule >> 
          *PatternFullRule
        ), 
        qi::space, 
        FlagsList, PatternList);
      
      // Ensure parsing succeeded
      if (!Parsed)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("FindPattern::LoadFileMemory") << 
          ErrorString("Parsing failed."));
      }
      
      // Ensure entire file was parsed
      if (DataBeg != DataEnd)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("FindPattern::LoadFileMemory") << 
          ErrorString("Parsing failed. Partial match only."));
      }
      
      // Get flags
      unsigned int Flags = None;
      std::for_each(FlagsList.cbegin(), FlagsList.cend(), 
        [&] (FindFlags Flag)
        {
          Flags |= Flag;
        });
      
      // Find all patterns
      std::for_each(PatternList.cbegin(), PatternList.cend(), 
        [&, this] (PatternInfoFull const& P)
        {
          // Create pattern scanner
          PatternInfo const& PatInfo = P.Pattern;
          Pattern MyPattern(*this, PatInfo.Data, PatInfo.Name, 
            static_cast<FindFlags>(Flags));
            
          // Apply manipulators
          std::vector<ManipInfo> const& ManipList = P.Manipulators;
          std::for_each(ManipList.cbegin(), ManipList.cend(), 
            [&] (ManipInfo const& M)
            {
              // Handle 'Add'
              if (M.Name == L"Add")
              {
                // Ensure correct operands are specified
                if (M.Operands.size() != 1)
                {
                  BOOST_THROW_EXCEPTION(FindPattern::Error() << 
                    ErrorFunction("FindPattern::LoadFileMemory") << 
                    ErrorString("Invalid manipulator operands for 'Add'."));
                }
                
                // Apply manipulator
                MyPattern << PatternManipulators::Add(M.Operands[0]);
              }
              // Handle 'Sub'
              else if (M.Name == L"Sub")
              {
                // Ensure correct operands are specified
                if (M.Operands.size() != 1)
                {
                  BOOST_THROW_EXCEPTION(FindPattern::Error() << 
                    ErrorFunction("FindPattern::LoadFileMemory") << 
                    ErrorString("Invalid manipulator operands for 'Sub'."));
                }
                
                // Apply manipulator
                MyPattern << PatternManipulators::Sub(M.Operands[0]);
              }
              // Handle 'Rel'
              else if (M.Name == L"Rel")
              {
                // Ensure correct operands are specified
                if (M.Operands.size() != 2)
                {
                  BOOST_THROW_EXCEPTION(FindPattern::Error() << 
                    ErrorFunction("FindPattern::LoadFileMemory") << 
                    ErrorString("Invalid manipulator operands for 'Rel'."));
                }
                
                // Apply manipulator
                MyPattern << PatternManipulators::Rel(M.Operands[0], 
                  M.Operands[1]);
              }
              // Handle 'Lea'
              else if (M.Name == L"Lea")
              {
                // Ensure correct operands are specified
                if (M.Operands.size() != 0)
                {
                  BOOST_THROW_EXCEPTION(FindPattern::Error() << 
                    ErrorFunction("FindPattern::LoadFileMemory") << 
                    ErrorString("Invalid manipulator operands for 'Lea'."));
                }
                
                // Apply manipulator
                MyPattern << PatternManipulators::Lea();
              }
            });
            
            // Save pattern back to parent
            MyPattern << PatternManipulators::Save();
        });
    }
    
    // Constructor
    Pattern::Pattern(FindPattern& Finder, std::wstring const& Data, 
	    std::wstring const& Name, FindPattern::FindFlags Flags)
	    : m_Finder(Finder), 
	    m_Name(Name), 
	    m_Address(static_cast<PBYTE>(Finder.Find(Data, Flags))), 
	    m_Flags(Flags)
	  { }
    
    // Constructor
    Pattern::Pattern(FindPattern& Finder, std::wstring const& Data, 
	    FindPattern::FindFlags Flags)
	    : m_Finder(Finder), 
	    m_Name(), 
	    m_Address(static_cast<PBYTE>(Finder.Find(Data, Flags))), 
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
      // Manipulate pattern
	    void Manipulator::Manipulate(Pattern& /*Pat*/) const
	    { }
  
      // Manipulator chaining operator overload
  		Pattern& operator<< (Pattern& Pat, Manipulator const& Manip)
  		{
  			Manip.Manipulate(Pat);
  			return Pat;
  		}
  		
      // Manipulate pattern
	    void Save::Manipulate(Pattern& Pat) const
	    {
        Pat.Save();
	    }
	    
      // Constructor
	    Add::Add(DWORD_PTR Offset)
	      : m_Offset(Offset)
	    { }
	    
      // Manipulate pattern
	    void Add::Manipulate(Pattern& Pat) const
	    {
        PBYTE Address = Pat.GetAddress();
        if (Address)
        {
          Pat.Update(Address + m_Offset);
        }
	    }
	    
      // Constructor
	    Sub::Sub(DWORD_PTR Offset)
	      : m_Offset(Offset)
	    { }
	    
      // Manipulate pattern
	    void Sub::Manipulate(Pattern& Pat) const
	    {
        PBYTE Address = Pat.GetAddress();
        if (Address)
        {
          Pat.Update(Address - m_Offset);
        }
	    }
	    
      // Manipulate pattern
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
	    
      // Constructor
	    Rel::Rel(DWORD_PTR Size, DWORD_PTR Offset)
	      : m_Size(Size), 
	      m_Offset(Offset)
	    { }
	    
      // Manipulate pattern
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

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
#include <boost/variant.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/numeric/conversion/cast.hpp>
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
      enum Manipulator
      {
        Manip_Add, 
        Manip_Sub, 
        Manip_Rel, 
        Manip_Lea
      };
      
      Manipulator Type;
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
  (std::wstring, Data))

// Adapt pattern manipulator info struct for parser
BOOST_FUSION_ADAPT_STRUCT(Hades::Memory::ManipInfo, 
  (Hades::Memory::ManipInfo::Manipulator, Type)
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
        [&, this] (Section const& S)
        {
          // Handle sections marked as code
          if ((S.GetCharacteristics() & IMAGE_SCN_CNT_CODE) == 
            IMAGE_SCN_CNT_CODE)
          {
            // Get start of section
            PBYTE SBegin = static_cast<PBYTE>(MyPeFile.RvaToVa(
              S.GetVirtualAddress()));
            if (SBegin == nullptr)
            {
              BOOST_THROW_EXCEPTION(FindPattern::Error() << 
                ErrorFunction("FindPattern::FindPattern") << 
                ErrorString("Could not get section base address."));
            }
            
            // Calculate end of section
            PBYTE SEnd = SBegin + S.GetSizeOfRawData();
            
            // Add section to list
            m_CodeRegions.push_back(std::make_pair(SBegin, SEnd));
          }
          
          // Handle sections marked as (initialized) data
          if ((S.GetCharacteristics() & IMAGE_SCN_CNT_INITIALIZED_DATA) == 
            IMAGE_SCN_CNT_INITIALIZED_DATA)
          {
            // Get start of section
            PBYTE SBegin = static_cast<PBYTE>(MyPeFile.RvaToVa(
              S.GetVirtualAddress()));
            if (SBegin == nullptr)
            {
              BOOST_THROW_EXCEPTION(FindPattern::Error() << 
                ErrorFunction("FindPattern::FindPattern") << 
                ErrorString("Could not get section base address."));
            }
            
            // Calculate end of section
            PBYTE SEnd = SBegin + S.GetSizeOfRawData();
            
            // Add section to list
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
      // For Boost.Spirit Qi
      namespace qi = boost::spirit::qi;
      
      // Data iterator type (wide string)
      typedef std::wstring::const_iterator DataIter;
      // Parser skipper type (whitespace)
      typedef qi::standard::space_type SkipWsT;
      
      // Data list entry parser
      boost::spirit::qi::rule<DataIter, unsigned int(), SkipWsT> DataRule;
      DataRule %= 
        (qi::hex | 
        qi::string(L"??")[qi::_val = static_cast<unsigned int>(-1)]);
      
      // Data list parser
      qi::rule<DataIter, std::vector<unsigned int>(), SkipWsT> DataListRule = 
        +(DataRule);
      
      // Parse data
      std::vector<unsigned int> DataParsed;
      auto DataBeg = Data.cbegin();
      auto DataEnd = Data.cend();
      bool Converted = boost::spirit::qi::phrase_parse(
        DataBeg, DataEnd, 
        DataListRule, 
        boost::spirit::qi::space, 
        DataParsed);
      if (!Converted || DataBeg != DataEnd)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("FindPattern::Find") << 
          ErrorString("Data parsing failed."));
      }
      
      // Convert data to required format
      std::vector<std::pair<BYTE, bool>> DataReal;
      std::transform(DataParsed.cbegin(), DataParsed.cend(), 
        std::back_inserter(DataReal), 
        [] (unsigned int Current) -> std::pair<BYTE, bool>
        {
          // Check for wildcard
          bool IsWildcard = (Current == static_cast<unsigned int>(-1));
          
          // Get data as integer
          BYTE CurrentByte = 0;
          if (!IsWildcard)
          {
            try
            {
              CurrentByte = boost::numeric_cast<BYTE>(Current);
            }
            catch (std::exception const& /*e*/)
            {
              BOOST_THROW_EXCEPTION(FindPattern::Error() << 
                ErrorFunction("FindPattern::Find") << 
                ErrorString("Data conversion failed (numeric)."));
            }
          }
    
          // Add current data and mask to list
          return std::make_pair(CurrentByte, !IsWildcard);
        });

      // Check if data sections should be scanned
      bool ScanDataSecs = ((Flags & FindFlags_ScanData) == 
        FindFlags_ScanData);
      
      // Find pattern
      PVOID Address = Find(DataReal, ScanDataSecs);
      
      // Throw on unmatched pattern if requested
      if (!Address && ((Flags & FindFlags_ThrowOnUnmatch) == 
        FindFlags_ThrowOnUnmatch))
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("FindPattern::Find") << 
          ErrorString("Could not match pattern."));
      }
      
      // Convert to relative address if required
      if (Address && ((Flags & FindFlags_RelativeAddress) == 
        FindFlags_RelativeAddress))
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
      // Data should always be non-empty
      BOOST_ASSERT(!Data.empty());
      
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
      // For Boost.Spirit Qi
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
            (L"None", FindFlags_None)
            (L"ThrowOnUnmatch", FindFlags_ThrowOnUnmatch)
            (L"RelativeAddress", FindFlags_RelativeAddress)
            (L"ScanData", FindFlags_ScanData);
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
      
      // Map manipulator names to values
      struct ManipParserT : qi::symbols<wchar_t, ManipInfo::Manipulator> 
      {
        ManipParserT()
        {
          add
            (L"Add", ManipInfo::Manip_Add)
            (L"Sub", ManipInfo::Manip_Sub)
            (L"Rel", ManipInfo::Manip_Rel)
            (L"Lea", ManipInfo::Manip_Lea);
        }
      } ManipParser;
      
      // Manipulator name parser
      qi::rule<DataIter, ManipInfo::Manipulator(), SkipWsT> ManipNameRule = 
        ManipParser >> ',';
      
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
      if (!Parsed || DataBeg != DataEnd)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("FindPattern::LoadFileMemory") << 
          ErrorString("Parsing failed."));
      }
      
      // Get flags
      unsigned int Flags = FindFlags_None;
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
              switch (M.Type)
              {
              // Handle 'Add'
              case ManipInfo::Manip_Add:
                // Ensure correct operands are specified
                if (M.Operands.size() != 1)
                {
                  BOOST_THROW_EXCEPTION(FindPattern::Error() << 
                    ErrorFunction("FindPattern::LoadFileMemory") << 
                    ErrorString("Invalid manipulator operands for 'Add'."));
                }
                
                // Apply manipulator
                MyPattern << PatternManipulators::Add(M.Operands[0]);
                  
                break;
                
              // Handle 'Sub'
              case ManipInfo::Manip_Sub:
                // Ensure correct operands are specified
                if (M.Operands.size() != 1)
                {
                  BOOST_THROW_EXCEPTION(FindPattern::Error() << 
                    ErrorFunction("FindPattern::LoadFileMemory") << 
                    ErrorString("Invalid manipulator operands for 'Sub'."));
                }
                
                // Apply manipulator
                MyPattern << PatternManipulators::Sub(M.Operands[0]);
                
                break;
              
              // Handle 'Rel'
              case ManipInfo::Manip_Rel:
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
                
                break;
              
              // Handle 'Lea'
              case ManipInfo::Manip_Lea:
                // Ensure correct operands are specified
                if (M.Operands.size() != 0)
                {
                  BOOST_THROW_EXCEPTION(FindPattern::Error() << 
                    ErrorFunction("FindPattern::LoadFileMemory") << 
                    ErrorString("Invalid manipulator operands for 'Lea'."));
                }
                
                // Apply manipulator
                MyPattern << PatternManipulators::Lea();
                
                break;
              
              // Handle unknown manipulators
              default:
                BOOST_THROW_EXCEPTION(FindPattern::Error() << 
                  ErrorFunction("FindPattern::LoadFileMemory") << 
                  ErrorString("Unknown manipulator."));
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
            DWORD_PTR Base = ((Pat.GetFlags() & 
              FindPattern::FindFlags_RelativeAddress) == 
              FindPattern::FindFlags_RelativeAddress) ? Pat.GetBase() : 0;
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
            DWORD_PTR Base = ((Pat.GetFlags() & 
              FindPattern::FindFlags_RelativeAddress) == 
              FindPattern::FindFlags_RelativeAddress) ? Pat.GetBase() : 0;
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

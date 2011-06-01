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

// C++ Standard Library
#include <fstream>
#include <sstream>

// Boost
#pragma warning(push, 1)
#pragma warning (disable: ALL_CODE_ANALYSIS_WARNINGS)
#include <boost/lexical_cast.hpp>
#pragma warning(pop)

// RapidXML
#pragma warning(push, 1)
#pragma warning (disable: ALL_CODE_ANALYSIS_WARNINGS)
#include <RapidXML/rapidxml.hpp>
#pragma warning(pop)

// Hades
#include "PeFile.h"
#include "Module.h"
#include "Scanner.h"
#include "DosHeader.h"
#include "NtHeaders.h"
#include "FindPattern.h"
#include "Hades-Common/I18n.h"

namespace Hades
{
  namespace Memory
  {
    // Constructor
    FindPattern::FindPattern(MemoryMgr const& MyMemory) 
      : m_Memory(MyMemory), 
      m_Start(nullptr), 
      m_End(nullptr), 
      m_Addresses()
    {
      // Get pointer to image headers
      ModuleListIter ModIter(m_Memory);
      PBYTE const pBase = reinterpret_cast<PBYTE>((*ModIter)->GetBase());
      PeFile MyPeFile(m_Memory, pBase);
      DosHeader const MyDosHeader(MyPeFile);
      NtHeaders const MyNtHeaders(MyPeFile);

      // Get base of code section
      m_Start = pBase + MyNtHeaders.GetBaseOfCode();

      // Calculate end of code section
      m_End = m_Start + MyNtHeaders.GetSizeOfCode();
    }

    // Constructor
    FindPattern::FindPattern(MemoryMgr const& MyMemory, HMODULE Module) 
      : m_Memory(MyMemory), 
      m_Start(nullptr), 
      m_End(nullptr), 
      m_Addresses()
    {
      // Ensure file is a valid PE file
      PBYTE const pBase = reinterpret_cast<PBYTE>(Module);
      PeFile MyPeFile(m_Memory, pBase);
      DosHeader const MyDosHeader(MyPeFile);
      NtHeaders const MyNtHeaders(MyPeFile);

      // Get base of code section
      m_Start = pBase + MyNtHeaders.GetBaseOfCode();

      // Calculate end of code section
      m_End = m_Start + MyNtHeaders.GetSizeOfCode();
    }

    // Find pattern
    PVOID FindPattern::Find(std::basic_string<TCHAR> const& Data, 
      std::basic_string<TCHAR> const& Mask) const
    {
      // Ensure pattern attributes are valid
      if (Data.empty() || Mask.empty())
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("FindPattern::Find") << 
          ErrorString("Empty pattern or mask data."));
      }

      // Ensure data is valid
      if (Data.size() % 2)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("FindPattern::Find") << 
          ErrorString("Data size invalid."));
      }

      // Ensure mask is valid
      if (Mask.size() * 2 != Data.size())
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("FindPattern::Find") << 
          ErrorString("Mask size invalid."));
      }

      // Convert data to byte buffer
      std::vector<std::pair<BYTE, bool>> DataBuf;
      for (auto i = Data.cbegin(), j = Mask.cbegin(); i != Data.cend(); 
        i += 2, ++j)
      {
        std::basic_string<TCHAR> const CurrentStr(i, i + 2);
        std::basic_stringstream<TCHAR> Converter(CurrentStr);
        int Current(0);
        if (!(Converter >> std::hex >> Current >> std::dec))
        {
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("FindPattern::Find") << 
            ErrorString("Invalid data conversion."));
        }

        BYTE CurrentReal = static_cast<BYTE>(Current);
        bool MaskFlag = *j == _T('x');

        DataBuf.push_back(std::make_pair(CurrentReal, MaskFlag));
      }

      // Search memory for pattern
      return Find(DataBuf);
    }

    // Search memory
    // Note: Previous implementation used modified Boyer-Moore-Horspool, but 
    // this was dropped as the C++ 'search' algorithm provided with MSVC 
    // performs equally if not slightly better than my custom search.
    PVOID FindPattern::Find(std::vector<std::pair<BYTE, bool>> const& Data) 
      const
    {
      // Cache all memory to be scanned
      std::size_t MemSize = m_End - m_Start;
      std::vector<BYTE> Buffer(m_Memory.Read<std::vector<BYTE>>(m_Start, 
        MemSize));

      // Scan memory
      auto Iter = std::search(Buffer.begin(), Buffer.end(), Data.begin(), 
        Data.end(), 
        [&] (BYTE HCur, std::pair<BYTE, bool> NCur)
      {
        return (!NCur.second) || (HCur == NCur.first);
      });

      // Return address if found or null if not found
      return 
        (Iter != Buffer.end()) 
        ? (m_Start + std::distance(Buffer.begin(), Iter)) 
        : nullptr;
    }

    // Load patterns from XML file
    void FindPattern::LoadFromXML(boost::filesystem::path const& Path)
    {
      // Open current file
      std::wifstream PatternFile(Path.string<std::basic_string<TCHAR>>().
        c_str());
      if (!PatternFile)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("FindPattern::LoadFromXML") << 
          ErrorString("Could not open pattern file."));
      }

      // Copy file to buffer
      std::istreambuf_iterator<wchar_t> const PatFileBeg(PatternFile);
      std::istreambuf_iterator<wchar_t> const PatFileEnd;
      std::vector<wchar_t> PatFileBuf(PatFileBeg, PatFileEnd);
      PatFileBuf.push_back(L'\0');

      // Open XML document
      std::shared_ptr<rapidxml::xml_document<wchar_t>> const AccountsDoc(
        std::make_shared<rapidxml::xml_document<wchar_t>>());
      AccountsDoc->parse<0>(&PatFileBuf[0]);

      // Ensure pattern tag is found
      rapidxml::xml_node<wchar_t>* PatternsTag = AccountsDoc->first_node(
        L"Patterns");
      if (!PatternsTag)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("FindPattern::LoadFromXML") << 
          ErrorString("Invalid pattern file format."));
      }

      // Loop over all patterns
      for (rapidxml::xml_node<wchar_t>* Pattern(PatternsTag->first_node(
        L"Pattern")); Pattern; Pattern = Pattern->next_sibling(L"Pattern"))
      {
        // Get pattern attributes
        rapidxml::xml_attribute<wchar_t> const* NameNode = Pattern->
          first_attribute(L"Name");
        rapidxml::xml_attribute<wchar_t> const* MaskNode = Pattern->
          first_attribute(L"Mask");
        rapidxml::xml_attribute<wchar_t> const* DataNode = Pattern->
          first_attribute(L"Data");
        std::wstring const Name(NameNode ? NameNode->value() : L"");
        std::wstring const Mask(MaskNode ? MaskNode->value() : L"");
        std::wstring const Data(DataNode ? DataNode->value() : L"");
        std::string const DataReal(boost::lexical_cast<std::string>(Data));

        // Ensure pattern attributes are valid
        if (Name.empty())
        {
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("FindPattern::LoadFromXML") << 
            ErrorString("Empty pattern name."));
        }

        // Find pattern
        PBYTE Address = static_cast<PBYTE>(Find(
          boost::lexical_cast<std::basic_string<TCHAR>>(Data), 
          boost::lexical_cast<std::basic_string<TCHAR>>(Mask)));

        // Only apply options if pattern was found
        if (Address != 0)
        {
          // Loop over all pattern options
          for (rapidxml::xml_node<wchar_t> const* PatOpts = Pattern->
            first_node(); PatOpts; PatOpts = PatOpts->next_sibling())
          {
            // Get option name
            std::wstring const OptionName(PatOpts->name());

            // Handle 'Add' and 'Sub' options
            bool const IsAdd = (OptionName == L"Add");
            bool const IsSub = (OptionName == L"Sub");
            if (IsAdd || IsSub)
            {
              // Get the modification value
              rapidxml::xml_attribute<wchar_t> const* ModVal = PatOpts->
                first_attribute(L"Value");
              if (!ModVal)
              {
                BOOST_THROW_EXCEPTION(Error() << 
                  ErrorFunction("FindPattern::LoadFromXML") << 
                  ErrorString("No value specified for 'Add' option."));
              }

              // Convert value to usable form
              std::wstringstream Converter(ModVal->value());
              DWORD_PTR AddValReal = 0;
              if (!(Converter >> std::hex >> AddValReal >> std::dec))
              {
                BOOST_THROW_EXCEPTION(Error() << 
                  ErrorFunction("FindPattern::LoadFromXML") << 
                  ErrorString("Invalid conversion for 'Add' option."));
              }

              // Perform modification
              if (IsAdd)
              {
                Address += AddValReal;
              }
              else if (IsSub)
              {
                Address -= AddValReal;
              }
              else
              {
                BOOST_THROW_EXCEPTION(Error() << 
                  ErrorFunction("FindPattern::LoadFromXML") << 
                  ErrorString("Unsupported pattern option."));
              }
            }
            // Handle 'Lea' option (abs deref)
            else if (OptionName == L"Lea")
            {
              // Perform absolute 'dereference'
              Address = m_Memory.Read<PBYTE>(Address);
            }
            // Handle 'Rel' option (rel deref)
            else if (OptionName == L"Rel")
            {
              // Get instruction size
              rapidxml::xml_attribute<wchar_t> const* SizeAttr = PatOpts->
                first_attribute(L"Size");
              if (!SizeAttr)
              {
                BOOST_THROW_EXCEPTION(Error() << 
                  ErrorFunction("FindPattern::LoadFromXML") << 
                  ErrorString("No size specified for 'Size' in 'Rel' "
                  "option."));
              }

              // Convert instruction size to usable format
              std::wstringstream SizeConverter(SizeAttr->value());
              DWORD_PTR Size(0);
              if (!(SizeConverter >> std::hex >> Size >> std::dec))
              {
                BOOST_THROW_EXCEPTION(Error() << 
                  ErrorFunction("FindPattern::LoadFromXML") << 
                  ErrorString("Invalid conversion for 'Size' in 'Rel' "
                  "option."));
              }

              // Get instruction offset
              rapidxml::xml_attribute<wchar_t> const* OffsetAttr = PatOpts->
                first_attribute(L"Offset");
              if (!OffsetAttr)
              {
                BOOST_THROW_EXCEPTION(Error() << 
                  ErrorFunction("FindPattern::LoadFromXML") << 
                  ErrorString("No value specified for 'Offset' in 'Rel' "
                  "option."));
              }

              // Convert instruction offset to usable format
              std::wstringstream OffsetConverter(OffsetAttr->value());
              DWORD_PTR Offset(0);
              if (!(OffsetConverter >> std::hex >> Offset >> std::dec))
              {
                BOOST_THROW_EXCEPTION(Error() << 
                  ErrorFunction("FindPattern::LoadFromXML") << 
                  ErrorString("Invalid conversion for 'Offset' in 'Rel' "
                  "option."));
              }

              // Perform relative 'dereference'
              Address = m_Memory.Read<PBYTE>(Address) + 
                reinterpret_cast<DWORD_PTR>(Address) + Size - Offset;
            }
            else
            {
              // Unknown pattern option
              BOOST_THROW_EXCEPTION(Error() << 
                ErrorFunction("FindPattern::LoadFromXML") << 
                ErrorString("Unknown pattern option."));
            }
          }
        }

        // Check for duplicate entry
        auto const Iter = m_Addresses.find(boost::lexical_cast<std::
          basic_string<TCHAR>>(Name));
        if (Iter != m_Addresses.end())
        {
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("FindPattern::LoadFromXML") << 
            ErrorString("Duplicate pattern name."));
        }

        // Add address to map
        m_Addresses[boost::lexical_cast<std::basic_string<TCHAR>>(Name)] = 
          Address;
      }
    }

    // Get address map
    std::map<std::basic_string<TCHAR>, PVOID> FindPattern::GetAddresses() const
    {
      return m_Addresses;
    }

    // Operator[] overload to allow retrieving addresses by name
    PVOID FindPattern::operator[](std::basic_string<TCHAR> const& Name) const
    {
      auto const Iter = m_Addresses.find(Name);
      return Iter != m_Addresses.end() ? Iter->second : nullptr;
    }
  }
}

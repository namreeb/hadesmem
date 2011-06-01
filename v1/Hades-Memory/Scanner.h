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
#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

// RapidXML
#include <RapidXML/rapidxml.hpp>

// Hades
#include "Module.h"
#include "Memory.h"
#include "Region.h"

namespace Hades
{
  namespace Memory
  {
    // Scanner exception type
    class ScannerError : public virtual HadesMemError 
    { };

    // Memory searching class
    class Scanner
    {
    public:
      // Constructor
      inline explicit Scanner(MemoryMgr const& MyMemory);
      inline explicit Scanner(MemoryMgr const& MyMemory, HMODULE Module);
      inline explicit Scanner(MemoryMgr const& MyMemory, PVOID Start, 
        PVOID End);

      // Search memory (POD types)
      template <typename T>
      PVOID Find(T const& Data, PVOID Start = 0, PVOID End = 0, 
        typename boost::enable_if<std::is_pod<T>>::type* Dummy = 0) const;

      // Search memory (string types)
      template <typename T>
      PVOID Find(T const& Data, PVOID Start = 0, PVOID End = 0, typename 
        boost::enable_if<std::is_same<T, std::basic_string<typename T::
        value_type>>>::type* Dummy = 0) const;

      // Search memory (vector types)
      template <typename T>
      PVOID Find(T const& Data, std::wstring const& Mask = L"", 
        PVOID Start = 0, PVOID End = 0, typename boost::enable_if<std::is_same<
        T, std::vector<typename T::value_type>>>::type* Dummy1 = 0, typename 
        boost::enable_if<std::is_pod<typename T::value_type>>::type* 
        Dummy2 = 0) const;

      // Search memory (POD types)
      template <typename T>
      std::vector<PVOID> FindAll(T const& data, PVOID Start = 0, PVOID End = 0, 
        typename boost::enable_if<std::is_pod<T>>::type* Dummy = 0) const;

      // Search memory (string types)
      template <typename T>
      std::vector<PVOID> FindAll(T const& Data, PVOID Start = 0, PVOID End = 0, 
        typename boost::enable_if<std::is_same<T, std::basic_string<typename T::
        value_type>>>::type* Dummy = 0) const;

      // Search memory (vector types)
      template <typename T>
      std::vector<PVOID> FindAll(T const& Data, std::wstring const& Mask = L"", 
        PVOID Start = 0, PVOID End = 0, typename boost::enable_if<std::is_same<
        T, std::vector<typename T::value_type>>>::type* Dummy1 = 0, typename 
        boost::enable_if<std::is_pod<typename T::value_type>>::type* Dummy2 = 
        0) const;

      // Load patterns from XML file
      inline void LoadFromXML(std::wstring const& Path);

      // Get address map
      inline std::map<std::wstring, PVOID> GetAddresses() const;

      // Operator[] overload to allow retrieving addresses by name
      inline PVOID operator[](std::wstring const& Name) const;

    private:
      // Disable assignment
      Scanner& operator= (Scanner const&);

      // Initialize using PE header
      void Initialize();

      // Memory manager instance
      MemoryMgr const& m_Memory;

      // Start and end addresses of search region
      PBYTE m_Start;
      PBYTE m_End;

      // Map to hold addresses
      std::map<std::wstring, PVOID> m_Addresses;
    };

    // Constructor
    Scanner::Scanner(MemoryMgr const& MyMemory, PVOID Start, PVOID End) 
      : m_Memory(MyMemory), 
      m_Start(static_cast<PBYTE>(Start)), 
      m_End(static_cast<PBYTE>(End)), 
      m_Addresses()
    {
      // Ensure range is valid
      if (m_End < m_Start)
      {
        BOOST_THROW_EXCEPTION(ScannerError() << 
          ErrorFunction("Scanner::Scanner") << 
          ErrorString("Start or end address is invalid."));
      }
    }

    // Constructor
    Scanner::Scanner(MemoryMgr const& MyMemory, HMODULE Module) 
      : m_Memory(MyMemory), 
      m_Start(nullptr), 
      m_End(nullptr), 
      m_Addresses()
    {
      // Ensure file is a valid PE file
      auto pBase = reinterpret_cast<PBYTE>(Module);
      auto DosHeader = MyMemory.Read<IMAGE_DOS_HEADER>(pBase);
      if (DosHeader.e_magic != IMAGE_DOS_SIGNATURE)
      {
        BOOST_THROW_EXCEPTION(ScannerError() << 
          ErrorFunction("Scanner::Scanner") << 
          ErrorString("Target file is not a valid PE file (DOS)."));
      }
      auto NtHeader = MyMemory.Read<IMAGE_NT_HEADERS>(pBase + DosHeader.
        e_lfanew);
      if (NtHeader.Signature != IMAGE_NT_SIGNATURE)
      {
        BOOST_THROW_EXCEPTION(ScannerError() << 
          ErrorFunction("Scanner::Scanner") << 
          ErrorString("Target file is not a valid PE file (NT)."));
      }

      // Get base of code section
      m_Start = pBase + NtHeader.OptionalHeader.BaseOfCode;

      // Calculate end of code section
      m_End = m_Start + NtHeader.OptionalHeader.SizeOfImage;
    }

    // Constructor
    Scanner::Scanner(MemoryMgr const& MyMemory) 
      : m_Memory(MyMemory), 
      m_Start(nullptr), 
      m_End(nullptr), 
      m_Addresses()
    {
      // Initialize using PE header
      Initialize();
    }

    // Search memory (POD types)
    template <typename T>
    PVOID Scanner::Find(T const& Data, PVOID Start, PVOID End, typename boost::
      enable_if<std::is_pod<T>>:: type* /*Dummy*/) const
    {
      // Put data in container
      std::vector<T> Buffer;
      Buffer.push_back(Data);
      // Use vector specialization of FindAll
      return Find(Buffer, L"", Start, End);
    }

    // Search memory (POD types)
    template <typename T>
    std::vector<PVOID> Scanner::FindAll(T const& Data, PVOID Start, PVOID End, 
      typename boost::enable_if<std::is_pod<T>>::type* /*Dummy*/) const
    {
      // Put data in container
      std::vector<T> Buffer;
      Buffer.push_back(Data);
      // Use vector specialization of FindAll
      return FindAll(Buffer, L"", Start, End);
    }

    // Search memory (string types)
    template <typename T>
    PVOID Scanner::Find(T const& Data, PVOID Start, PVOID End, typename boost::
      enable_if<std::is_same<T, std::basic_string<typename T::value_type>>>::
      type* /*Dummy*/) const
    {
      // Convert string to character buffer
      std::vector<T::value_type> MyBuffer(Data.begin(), Data.end());
      // Use vector specialization of find
      return Find(MyBuffer, L"", Start, End);
    }

    template <typename T>
    std::vector<PVOID> Scanner::FindAll(T const& Data, PVOID Start, PVOID End, 
      typename boost::enable_if<std::is_same<T, std::basic_string<typename T::
      value_type>>>::type* /*Dummy*/) const
    {
      // Convert string to character buffer
      std::vector<T::value_type> MyBuffer(Data.begin(), Data.end());
      // Use vector specialization of find all
      return FindAll(MyBuffer, L"", Start, End);
    }

    // Search memory (vector types)
    template <typename T>
    PVOID Scanner::Find(T const& Data, std::wstring const& Mask, PVOID Start, 
      PVOID End, typename boost::enable_if<std::is_same<T, std::vector<
      typename T::value_type>>>::type* /*Dummy1*/, typename boost::enable_if<
      std::is_pod<typename T::value_type>>::type* /*Dummy2*/) const
    {
      // Ensure there is data to process
      if (Data.empty())
      {
        BOOST_THROW_EXCEPTION(MemoryError() << 
          ErrorFunction("MemoryMgr::Find") << 
          ErrorString("Mask does not match data."));
      }

      // Ensure mask matches data
      if (!Mask.empty() && Mask.size() != Data.size())
      {
        BOOST_THROW_EXCEPTION(MemoryError() << 
          ErrorFunction("MemoryMgr::Find") << 
          ErrorString("Mask does not match data."));
      }

      // Get real start and end addresses
      PBYTE const StartReal = (Start || End) ? static_cast<PBYTE>(Start) : 
        m_Start;
      PBYTE const EndReal = (Start || End) ? static_cast<PBYTE>(End) : m_End;
      if (EndReal < StartReal)
      {
        BOOST_THROW_EXCEPTION(ScannerError() << 
          ErrorFunction("Scanner::Find") << 
          ErrorString("Start or end address is invalid."));
      }

      // Get system information
      SYSTEM_INFO MySystemInfo = { 0 };
      GetSystemInfo(&MySystemInfo);
      DWORD const PageSize = MySystemInfo.dwPageSize;
      PVOID const MinAddr = MySystemInfo.lpMinimumApplicationAddress;
      PVOID const MaxAddr = MySystemInfo.lpMaximumApplicationAddress;

      // Loop over all memory pages
      for (auto Address = static_cast<PBYTE>(MinAddr); Address < MaxAddr; 
        Address += PageSize)
      {
        try
        {
          // Skip region if out of bounds
          if (Address + PageSize < StartReal)
          {
            continue;
          }

          // Quit if out of bounds
          if (Address > EndReal)
          {
            break;
          }

          // Check for invalid memory
          MEMORY_BASIC_INFORMATION MyMbi1 = { 0 };
          if (!VirtualQueryEx(m_Memory.GetProcessHandle(), Address, &MyMbi1, 
            sizeof(MyMbi1)) || (MyMbi1.Protect & PAGE_GUARD) == PAGE_GUARD)
          {
            continue;
          }

          // Check for invalid memory
          MEMORY_BASIC_INFORMATION MyMbi2 = { 0 };
          if (!VirtualQueryEx(m_Memory.GetProcessHandle(), Address + PageSize, 
            &MyMbi2, sizeof(MyMbi2)) || (MyMbi2.Protect & PAGE_GUARD) == 
            PAGE_GUARD)
          {
            continue;
          }

          // Read vector of Ts into cache
          // Todo: If we're reading across a region boundary and we hit 
          // inaccessible memory we should simply read all we can, rather 
          // than skipping the block entirely.
          auto const Buffer(m_Memory.Read<std::vector<BYTE>>(Address, 
            PageSize + Data.size() * sizeof(T::value_type)));

          // Loop over entire memory region
          for (auto Current = &Buffer[0]; Current != &Buffer[0] + 
            Buffer.size(); ++Current) 
          {
            // Check if current address matches buffer
            bool Found = true;
            for (std::vector<BYTE>::size_type i = 0; i != Data.size(); ++i)
            {
              auto CurrentTemp = reinterpret_cast<T::value_type const* const>(
                Current);
              if ((Mask.empty() || Mask[i] == L'x') && 
                (CurrentTemp[i] != Data[i]))
              {
                Found = false;
                break;
              }
            }

            // If the buffer matched return the current address
            if (Found)
            {
              // If the buffer matched and the address is valid, return the 
              // current address.
              // Todo: Do this check in the outer loop, and break if possible 
              // rather than continuing.
              PVOID const AddressReal = Address + (Current - &Buffer[0]);
              if (AddressReal >= StartReal && AddressReal <= EndReal)
              {
                return AddressReal;
              }
            }
          }
        }
        // Ignore any memory errors, as there's nothing we can do about them
        // Todo: Detect memory read errors and drop back to a slower but 
        // more reliable implementation.
        catch (MemoryError const& /*e*/)
        {
          continue;
        }
      }

      // Nothing found, return null
      return nullptr;
    }

    template <typename T>
    std::vector<PVOID> Scanner::FindAll(T const& Data, 
      std::wstring const& Mask, PVOID Start, PVOID End, typename boost::
      enable_if<std::is_same<T, std::vector<typename T::value_type>>>::type* 
      /*Dummy1*/, typename boost::enable_if<std::is_pod<typename T::
      value_type>>::type* /*Dummy2*/) const
    {
      // Ensure there is data to process
      if (Data.empty())
      {
        BOOST_THROW_EXCEPTION(MemoryError() << 
          ErrorFunction("MemoryMgr::Find") << 
          ErrorString("Mask does not match data."));
      }

      // Ensure mask matches data
      if (!Mask.empty() && Mask.size() != Data.size())
      {
        BOOST_THROW_EXCEPTION(MemoryError() << 
          ErrorFunction("MemoryMgr::Find") << 
          ErrorString("Mask does not match data."));
      }

      // Get real start and end addresses
      PBYTE const StartReal = Start || End ? static_cast<PBYTE>(Start) : 
        m_Start;
      PBYTE const EndReal = Start || End ? static_cast<PBYTE>(End) : m_End;
      if (EndReal < StartReal)
      {
        BOOST_THROW_EXCEPTION(ScannerError() << 
          ErrorFunction("Scanner::Find") << 
          ErrorString("Start or end address is invalid."));
      }

      // Addresses of matches
      std::vector<PVOID> Matches;

      // Get system information
      SYSTEM_INFO MySystemInfo = { 0 };
      GetSystemInfo(&MySystemInfo);
      DWORD const PageSize = MySystemInfo.dwPageSize;
      PVOID const MinAddr = MySystemInfo.lpMinimumApplicationAddress;
      PVOID const MaxAddr = MySystemInfo.lpMaximumApplicationAddress;

      // Loop over all memory pages
      for (auto Address = static_cast<PBYTE>(MinAddr); Address < MaxAddr; 
        Address += PageSize)
      {
        try
        {
          // Skip region if out of bounds
          if (Address + PageSize < StartReal)
          {
            continue;
          }

          // Quit if out of bounds
          if (Address > EndReal)
          {
            break;
          }

          // Check for invalid memory
          MEMORY_BASIC_INFORMATION MyMbi1 = { 0 };
          if (!VirtualQueryEx(m_Memory.GetProcessHandle(), Address, &MyMbi1, 
            sizeof(MyMbi1)) || (MyMbi1.Protect & PAGE_GUARD) == PAGE_GUARD)
          {
            continue;
          }

          // Check for invalid memory
          MEMORY_BASIC_INFORMATION MyMbi2 = { 0 };
          if (!VirtualQueryEx(m_Memory.GetProcessHandle(), Address + PageSize, 
            &MyMbi2, sizeof(MyMbi2)) || (MyMbi2.Protect & PAGE_GUARD) == 
            PAGE_GUARD)
          {
            continue;
          }

          // Read vector of Ts into cache
          // Todo: If we're reading across a region boundary and we hit 
          // inaccessible memory we should simply read all we can, rather 
          // than skipping the block entirely.
          auto const Buffer(m_Memory.Read<std::vector<BYTE>>(Address, 
            PageSize + Data.size() * sizeof(T::value_type)));

          // Loop over entire memory region
          for (auto Current = &Buffer[0]; Current != &Buffer[0] + 
            Buffer.size(); ++Current) 
          {
            // Check if current address matches buffer
            bool Found = true;
            for (std::vector<BYTE>::size_type i = 0; i != Data.size(); ++i)
            {
              auto CurrentTemp = reinterpret_cast<T::value_type const* const>(
                Current);
              if ((Mask.empty() || Mask[i] == L'x') && 
                (CurrentTemp[i] != Data[i]))
              {
                Found = false;
                break;
              }
            }

            // If the buffer matched return the current address
            if (Found)
            {
              // If the buffer matched and the address is valid, return the 
              // current address.
              // Todo: Do this check in the outer loop, and break if possible 
              // rather than continuing.
              PVOID const AddressReal = Address + (Current - &Buffer[0]);
              if (AddressReal >= StartReal && AddressReal <= EndReal)
              {
                Matches.push_back(AddressReal);
              }
            }
          }
        }
        // Ignore any memory errors, as there's nothing we can do about them
        // Todo: Detect memory read errors and drop back to a slower but 
        // more reliable implementation.
        catch (MemoryError const& /*e*/)
        {
          continue;
        }
      }

      // Return matches
      return Matches;
    }

    // Load patterns from XML file
    void Scanner::LoadFromXML(std::wstring const& Path)
    {
      // Open current file
      std::wifstream PatternFile(Path.c_str());
      if (!PatternFile)
      {
        BOOST_THROW_EXCEPTION(ScannerError() << 
          ErrorFunction("Scanner::LoadFromXML") << 
          ErrorString("Could not open pattern file."));
      }

      // Copy file to buffer
      std::istreambuf_iterator<wchar_t> PatFileBeg(PatternFile);
      std::istreambuf_iterator<wchar_t> PatFileEnd;
      std::vector<wchar_t> PatFileBuf(PatFileBeg, PatFileEnd);
      PatFileBuf.push_back(L'\0');

      // Open XML document
      rapidxml::xml_document<wchar_t> AccountsDoc;
      AccountsDoc.parse<0>(&PatFileBuf[0]);

      // Ensure pattern tag is found
      auto PatternsTag = AccountsDoc.first_node(L"Patterns");
      if (!PatternsTag)
      {
        BOOST_THROW_EXCEPTION(ScannerError() << 
          ErrorFunction("Scanner::LoadFromXML") << 
          ErrorString("Invalid pattern file format."));
      }

      // Loop over all patterns
      for (auto Pattern = PatternsTag->first_node(L"Pattern"); Pattern; 
        Pattern = Pattern->next_sibling(L"Pattern"))
      {
        // Get pattern attributes
        auto NameNode = Pattern->first_attribute(L"Name");
        auto MaskNode = Pattern->first_attribute(L"Mask");
        auto DataNode = Pattern->first_attribute(L"Data");
        std::wstring Name(NameNode ? NameNode->value() : L"");
        std::wstring Mask(MaskNode ? MaskNode->value() : L"");
        std::wstring Data(DataNode ? DataNode->value() : L"");
        std::string DataReal(boost::lexical_cast<std::string>(Data));

        // Ensure pattern attributes are valid
        if (Name.empty() || Mask.empty() || Data.empty())
        {
          BOOST_THROW_EXCEPTION(ScannerError() << 
            ErrorFunction("Scanner::LoadFromXML") << 
            ErrorString("Invalid pattern attributes."));
        }

        // Ensure data is valid
        if (Data.size() % 2)
        {
          BOOST_THROW_EXCEPTION(ScannerError() << 
            ErrorFunction("Scanner::LoadFromXML") << 
            ErrorString("Data size invalid."));
        }

        // Ensure mask is valid
        if (Mask.size() * 2 != Data.size())
        {
          BOOST_THROW_EXCEPTION(ScannerError() << 
            ErrorFunction("Scanner::LoadFromXML") << 
            ErrorString("Mask size invalid."));
        }

        // Convert data to byte buffer
        std::vector<BYTE> DataBuf;
        for (auto i = DataReal.begin(); i != DataReal.end(); i += 2)
        {
          std::string CurrentStr(i, i + 2);
          std::stringstream Converter(CurrentStr);
          int Current = 0;
          if (!(Converter >> std::hex >> Current >> std::dec))
          {
            BOOST_THROW_EXCEPTION(ScannerError() << 
              ErrorFunction("Scanner::LoadFromXML") << 
              ErrorString("Invalid data conversion."));
          }
          DataBuf.push_back(static_cast<BYTE>(Current));
        }

        // Find pattern
        PBYTE Address = static_cast<PBYTE>(Find(DataBuf, Mask));

        // Only apply options if pattern was found
        if (Address != 0)
        {
          // Loop over all pattern options
          for (auto PatOpts = Pattern->first_node(); PatOpts; 
            PatOpts = PatOpts->next_sibling())
          {
            // Get option name
            std::wstring OptionName(PatOpts->name());

            // Handle 'Add' and 'Sub' options
            bool IsAdd = std::wstring(OptionName) == L"Add";
            bool IsSub = std::wstring(OptionName) == L"Sub";
            if (IsAdd || IsSub)
            {
              // Get the modification value
              auto ModVal = PatOpts->first_attribute(L"Value");
              if (!ModVal)
              {
                BOOST_THROW_EXCEPTION(ScannerError() << 
                  ErrorFunction("Scanner::LoadFromXML") << 
                  ErrorString("No value specified for 'Add' option."));
              }

              // Convert value to usable form
              DWORD_PTR AddValReal = 0;
              std::wstringstream Converter(ModVal->value());
              if (!(Converter >> std::hex >> AddValReal >> std::dec))
              {
                BOOST_THROW_EXCEPTION(ScannerError() << 
                  ErrorFunction("Scanner::LoadFromXML") << 
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
                BOOST_THROW_EXCEPTION(ScannerError() << 
                  ErrorFunction("Scanner::LoadFromXML") << 
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
              auto SizeAttr = PatOpts->first_attribute(L"Size");
              if (!SizeAttr)
              {
                BOOST_THROW_EXCEPTION(ScannerError() << 
                  ErrorFunction("Scanner::LoadFromXML") << 
                  ErrorString("No size specified for 'Size' in 'Rel' "
                  "option."));
              }

              // Convert instruction size to usable format
              DWORD_PTR Size = 0;
              std::wstringstream SizeConverter(SizeAttr->value());
              if (!(SizeConverter >> std::hex >> Size >> std::dec))
              {
                BOOST_THROW_EXCEPTION(ScannerError() << 
                  ErrorFunction("Scanner::LoadFromXML") << 
                  ErrorString("Invalid conversion for 'Size' in 'Rel' "
                  "option."));
              }

              // Get instruction offset
              auto OffsetAttr = PatOpts->first_attribute(L"Offset");
              if (!OffsetAttr)
              {
                BOOST_THROW_EXCEPTION(ScannerError() << 
                  ErrorFunction("Scanner::LoadFromXML") << 
                  ErrorString("No value specified for 'Offset' in 'Rel' "
                  "option."));
              }

              // Convert instruction onffset to usable format
              DWORD_PTR Offset = 0;
              std::wstringstream OffsetConverter(OffsetAttr->value());
              if (!(OffsetConverter >> std::hex >> Offset >> std::dec))
              {
                BOOST_THROW_EXCEPTION(ScannerError() << 
                  ErrorFunction("Scanner::LoadFromXML") << 
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
              BOOST_THROW_EXCEPTION(ScannerError() << 
                ErrorFunction("Scanner::LoadFromXML") << 
                ErrorString("Unknown pattern option."));
            }
          }
        }

        // Add address to map
        m_Addresses[Name] = Address;
      }
    }

    // Get address map
    std::map<std::wstring, PVOID> Scanner::GetAddresses() const
    {
      return m_Addresses;
    }

    // Initialize using PE header
    void Scanner::Initialize()
    {
      // Get module list
      auto ModuleList = GetModuleList(m_Memory);

      // Ensure module list is valid
      if (ModuleList.empty())
      {
        BOOST_THROW_EXCEPTION(ScannerError() << 
          ErrorFunction("Scanner::Scanner") << 
          ErrorString("Could not get module list."));
      }

      // Get pointer to image headers
      auto pBase = reinterpret_cast<PBYTE>(ModuleList[0]->GetBase());
      auto DosHeader = m_Memory.Read<IMAGE_DOS_HEADER>(pBase);
      auto NtHeader = m_Memory.Read<IMAGE_NT_HEADERS>(pBase + DosHeader.
        e_lfanew);

      // Get base of code section
      m_Start = pBase + NtHeader.OptionalHeader.BaseOfCode;

      // Calculate end of code section
      m_End = m_Start + NtHeader.OptionalHeader.SizeOfImage;
    }

    // Operator[] overload to allow retrieving addresses by name
    PVOID Scanner::operator[](std::wstring const& Name) const
    {
      auto Iter = m_Addresses.find(Name);
      return Iter != m_Addresses.end() ? Iter->second : nullptr;
    }
  }
}

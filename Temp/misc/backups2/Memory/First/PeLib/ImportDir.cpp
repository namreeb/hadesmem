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
#include <HadesMemory/PeLib/ImportDir.hpp>
#include <HadesMemory/MemoryMgr.hpp>
#include <HadesMemory/PeLib/PeFile.hpp>
#include <HadesMemory/PeLib/NtHeaders.hpp>
#include <HadesMemory/PeLib/DosHeader.hpp>

// C++ Standard Library
#include <vector>

namespace Hades
{
  namespace Memory
  {
    // Constructor
    ImportDir::ImportDir(PeFile const& MyPeFile, 
      PIMAGE_IMPORT_DESCRIPTOR pImpDesc) 
      : m_PeFile(MyPeFile), 
      m_Memory(m_PeFile.GetMemoryMgr()), 
      m_pImpDesc(pImpDesc)
    { }

    // Whether import directory is valid
    bool ImportDir::IsValid() const
    {
      // Get NT headers
      NtHeaders const MyNtHeaders(m_PeFile);

      // Get import dir data
      DWORD const DataDirSize(MyNtHeaders.GetDataDirectorySize(NtHeaders::
        DataDir_Import));
      DWORD const DataDirVa(MyNtHeaders.GetDataDirectoryVirtualAddress(
        NtHeaders::DataDir_Import));

      // Import dir is valid if size and rva are valid
      return DataDirSize && DataDirVa;
    }

    // Ensure import directory is valid
    void ImportDir::EnsureValid() const
    {
      if (!IsValid())
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("ImportDir::EnsureValid") << 
          ErrorString("Import directory is invalid."));
      }
    }

    // Get import directory base
    PBYTE ImportDir::GetBase() const
    {
      // Initialize base address if necessary
      if (!m_pImpDesc)
      {
        // Get NT headers
        NtHeaders const MyNtHeaders(m_PeFile);

        // Get import dir data
        DWORD const DataDirSize = MyNtHeaders.GetDataDirectorySize(NtHeaders::
          DataDir_Import);
        DWORD const DataDirVa = MyNtHeaders.GetDataDirectoryVirtualAddress(
          NtHeaders::DataDir_Import);
        if (!DataDirSize || !DataDirVa)
        {
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("ImportDir::GetBase") << 
            ErrorString("PE file has no import directory."));
        }

        // Init base address
        m_pImpDesc = static_cast<PIMAGE_IMPORT_DESCRIPTOR>(m_PeFile.RvaToVa(
          DataDirVa));
      }

      // Return base address
      return reinterpret_cast<PBYTE>(m_pImpDesc);
    }

    // Advance to next descriptor
    void ImportDir::Advance() const
    {
      ++m_pImpDesc;
    }

    // Get characteristics
    DWORD ImportDir::GetCharacteristics() const
    {
      return m_Memory.Read<DWORD>(GetBase() + FIELD_OFFSET(
        IMAGE_IMPORT_DESCRIPTOR, Characteristics));
    }

    // Set characteristics
    void ImportDir::SetCharacteristics(DWORD Characteristics) const
    {
      return m_Memory.Write(GetBase() + FIELD_OFFSET(IMAGE_IMPORT_DESCRIPTOR, 
        Characteristics), Characteristics);
    }

    // Get time and date stamp
    DWORD ImportDir::GetTimeDateStamp() const
    {
      return m_Memory.Read<DWORD>(GetBase() + FIELD_OFFSET(
        IMAGE_IMPORT_DESCRIPTOR, TimeDateStamp));
    }

    // Set time and date stamp
    void ImportDir::SetTimeDateStamp(DWORD TimeDateStamp) const
    {
      return m_Memory.Write(GetBase() + FIELD_OFFSET(IMAGE_IMPORT_DESCRIPTOR, 
        TimeDateStamp), TimeDateStamp);
    }

    // Get forwarder chain
    DWORD ImportDir::GetForwarderChain() const
    {
      return m_Memory.Read<DWORD>(GetBase() + FIELD_OFFSET(
        IMAGE_IMPORT_DESCRIPTOR, ForwarderChain));
    }

    // Set forwarder chain
    void ImportDir::SetForwarderChain(DWORD ForwarderChain) const
    {
      return m_Memory.Write(GetBase() + FIELD_OFFSET(IMAGE_IMPORT_DESCRIPTOR, 
        ForwarderChain), ForwarderChain);
    }

    // Get name (raw)
    DWORD ImportDir::GetNameRaw() const
    {
      return m_Memory.Read<DWORD>(GetBase() + FIELD_OFFSET(
        IMAGE_IMPORT_DESCRIPTOR, Name));
    }

    // Set name (raw)
    void ImportDir::SetNameRaw(DWORD Name) const
    {
      return m_Memory.Write(GetBase() + FIELD_OFFSET(IMAGE_IMPORT_DESCRIPTOR, 
        Name), Name);
    }

    // Get name
    std::string ImportDir::GetName() const
    {
      return m_Memory.Read<std::string>(m_PeFile.RvaToVa(GetNameRaw()));
    }

    // Get first thunk
    DWORD ImportDir::GetFirstThunk() const
    {
      return m_Memory.Read<DWORD>(GetBase() + FIELD_OFFSET(
        IMAGE_IMPORT_DESCRIPTOR, FirstThunk));
    }

    // Set first thunk
    void ImportDir::SetFirstThunk(DWORD FirstThunk) const
    {
      return m_Memory.Write(GetBase() + FIELD_OFFSET(IMAGE_IMPORT_DESCRIPTOR, 
        FirstThunk), FirstThunk);
    }

    // Constructor
    ImportThunk::ImportThunk(PeFile const& MyPeFile, PVOID pThunk) 
      : m_PeFile(MyPeFile), 
      m_Memory(MyPeFile.GetMemoryMgr()), 
      m_pThunk(static_cast<PIMAGE_THUNK_DATA>(pThunk)), 
      m_pBase(reinterpret_cast<PBYTE>(m_pThunk))
    { }
    
    // Whether thunk is valid
    bool ImportThunk::IsValid() const
    {
      return GetAddressOfData() != 0;
    }

    // Ensure thunk is valid
    void ImportThunk::EnsureValid() const
    {
      if (!IsValid())
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("ImportThunk::EnsureValid") << 
          ErrorString("Import thunk is invalid."));
      }
    }

    // Advance to next thunk
    void ImportThunk::Advance() const
    {
      ++m_pThunk;
      m_pBase = reinterpret_cast<PBYTE>(m_pThunk);
    }

    // Get address of data
    DWORD_PTR ImportThunk::GetAddressOfData() const
    {
      return m_Memory.Read<DWORD_PTR>(m_pBase + FIELD_OFFSET(
        IMAGE_THUNK_DATA, u1.AddressOfData));
    }

    // Set address of data
    void ImportThunk::SetAddressOfData(DWORD_PTR AddressOfData) const
    {
      return m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_THUNK_DATA, 
        u1.AddressOfData), AddressOfData);
    }

    // Get ordinal (raw)
    DWORD_PTR ImportThunk::GetOrdinalRaw() const
    {
      return m_Memory.Read<DWORD_PTR>(m_pBase + FIELD_OFFSET(
        IMAGE_THUNK_DATA, u1.Ordinal));
    }

    // Set ordinal (raw)
    void ImportThunk::SetOrdinalRaw(DWORD_PTR OrdinalRaw) const
    {
      return m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_THUNK_DATA, 
        u1.Ordinal), OrdinalRaw);
    }

    // Whether import is by ordinal
    bool ImportThunk::ByOrdinal() const
    {
      return IMAGE_SNAP_BY_ORDINAL(GetOrdinalRaw());
    }

    // Get ordinal
    WORD ImportThunk::GetOrdinal() const
    {
      return IMAGE_ORDINAL(GetOrdinalRaw());
    }

    // Get function
    DWORD_PTR ImportThunk::GetFunction() const
    {
      return m_Memory.Read<DWORD_PTR>(m_pBase + FIELD_OFFSET(
        IMAGE_THUNK_DATA, u1.Function));
    }

    // Set function
    void ImportThunk::SetFunction(DWORD_PTR Function) const
    {
      return m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_THUNK_DATA, 
        u1.Function), Function);
    }

    // Get hint
    WORD ImportThunk::GetHint() const
    {
      PBYTE const pNameImport = static_cast<PBYTE>(m_PeFile.RvaToVa(
        static_cast<DWORD>(GetAddressOfData())));
      return m_Memory.Read<WORD>(pNameImport + FIELD_OFFSET(
        IMAGE_IMPORT_BY_NAME, Hint));
    }

    // Set hint
    void ImportThunk::SetHint(WORD Hint) const
    {
      PBYTE const pNameImport = static_cast<PBYTE>(m_PeFile.RvaToVa(
        static_cast<DWORD>(GetAddressOfData())));
      return m_Memory.Write(pNameImport + FIELD_OFFSET(IMAGE_IMPORT_BY_NAME, 
        Hint), Hint);
    }

    // Get name
    std::string ImportThunk::GetName() const
    {
      PBYTE const pNameImport = static_cast<PBYTE>(m_PeFile.RvaToVa(
        static_cast<DWORD>(GetAddressOfData())));
      return m_Memory.Read<std::string>(pNameImport + FIELD_OFFSET(
        IMAGE_IMPORT_BY_NAME, Name));
    }
    
    // Get base
    PVOID ImportThunk::GetBase() const
    {
      return m_pBase;
    }
  }
}

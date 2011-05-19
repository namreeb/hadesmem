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
#include "HadesMemory/MemoryMgr.hpp"
#include "HadesMemory/PeLib/TlsDir.hpp"
#include "HadesMemory/PeLib/PeFile.hpp"
#include "HadesMemory/PeLib/NtHeaders.hpp"

namespace Hades
{
  namespace Memory
  {
    // Constructor
    TlsDir::TlsDir(PeFile const& MyPeFile)
      : m_PeFile(MyPeFile), 
      m_Memory(m_PeFile.GetMemoryMgr()), 
      m_pBase(nullptr)
    { }

    // Whether TLS directory is valid
    bool TlsDir::IsValid() const
    {
      // Get NT headers
      NtHeaders const MyNtHeaders(m_PeFile);

      // Get TLS dir data
      DWORD const DataDirSize(MyNtHeaders.GetDataDirectorySize(NtHeaders::
        DataDir_TLS));
      DWORD const DataDirVa(MyNtHeaders.GetDataDirectoryVirtualAddress(
        NtHeaders::DataDir_TLS));

      // TLS dir is valid if size and rva are valid
      return DataDirSize && DataDirVa;
    }

    // Ensure export directory is valid
    void TlsDir::EnsureValid() const
    {
      if (!IsValid())
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("TlsDir::EnsureValid") << 
          ErrorString("TLS directory is invalid."));
      }
    }

    // Get start address of raw data
    DWORD_PTR TlsDir::GetStartAddressOfRawData() const
    {
      PBYTE const pBase = static_cast<PBYTE>(GetBase());
      return m_Memory.Read<DWORD_PTR>(pBase + FIELD_OFFSET(
        IMAGE_TLS_DIRECTORY, StartAddressOfRawData));
    }

    // Set start address of raw data
    void TlsDir::SetStartAddressOfRawData(DWORD_PTR StartAddressOfRawData) const
    {
      PBYTE const pBase = static_cast<PBYTE>(GetBase());
      m_Memory.Write(pBase + FIELD_OFFSET(IMAGE_TLS_DIRECTORY, 
        StartAddressOfRawData), StartAddressOfRawData);
    }

    // Get end address of raw data
    DWORD_PTR TlsDir::GetEndAddressOfRawData() const
    {
      PBYTE const pBase = static_cast<PBYTE>(GetBase());
      return m_Memory.Read<DWORD_PTR>(pBase + FIELD_OFFSET(
        IMAGE_TLS_DIRECTORY, EndAddressOfRawData));
    }

    // Set end address of raw data
    void TlsDir::SetEndAddressOfRawData(DWORD_PTR EndAddressOfRawData) const
    {
      PBYTE const pBase = static_cast<PBYTE>(GetBase());
      m_Memory.Write(pBase + FIELD_OFFSET(IMAGE_TLS_DIRECTORY, 
        EndAddressOfRawData), EndAddressOfRawData);
    }

    // Get address of index
    DWORD_PTR TlsDir::GetAddressOfIndex() const
    {
      PBYTE const pBase = static_cast<PBYTE>(GetBase());
      return m_Memory.Read<DWORD_PTR>(pBase + FIELD_OFFSET(
        IMAGE_TLS_DIRECTORY, AddressOfIndex));
    }

    // Set address of index
    void TlsDir::SetAddressOfIndex(DWORD_PTR AddressOfIndex) const
    {
      PBYTE const pBase = static_cast<PBYTE>(GetBase());
      m_Memory.Write(pBase + FIELD_OFFSET(IMAGE_TLS_DIRECTORY, 
        AddressOfIndex), AddressOfIndex);
    }

    // Get address of callbacks
    DWORD_PTR TlsDir::GetAddressOfCallBacks() const
    {
      PBYTE const pBase = static_cast<PBYTE>(GetBase());
      return m_Memory.Read<DWORD_PTR>(pBase + FIELD_OFFSET(
        IMAGE_TLS_DIRECTORY, AddressOfCallBacks));
    }

    // Set address of callbacks
    void TlsDir::SetAddressOfCallBacks(DWORD_PTR AddressOfCallBacks) const
    {
      PBYTE const pBase = static_cast<PBYTE>(GetBase());
      m_Memory.Write(pBase + FIELD_OFFSET(IMAGE_TLS_DIRECTORY, 
        AddressOfCallBacks), AddressOfCallBacks);
    }

    // Get size of zero fill
    DWORD TlsDir::GetSizeOfZeroFill() const
    {
      PBYTE const pBase = static_cast<PBYTE>(GetBase());
      return m_Memory.Read<DWORD>(pBase + FIELD_OFFSET(
        IMAGE_TLS_DIRECTORY, SizeOfZeroFill));
    }

    // Set size of zero fill
    void TlsDir::SetSizeOfZeroFill(DWORD SizeOfZeroFill) const
    {
      PBYTE const pBase = static_cast<PBYTE>(GetBase());
      m_Memory.Write(pBase + FIELD_OFFSET(IMAGE_TLS_DIRECTORY, 
        SizeOfZeroFill), SizeOfZeroFill);
    }

    // Get characteristics
    DWORD TlsDir::GetCharacteristics() const
    {
      PBYTE const pBase = static_cast<PBYTE>(GetBase());
      return m_Memory.Read<DWORD>(pBase + FIELD_OFFSET(
        IMAGE_TLS_DIRECTORY, Characteristics));
    }

    // Set characteristics
    void TlsDir::SetCharacteristics(DWORD Characteristics) const
    {
      PBYTE const pBase = static_cast<PBYTE>(GetBase());
      m_Memory.Write(pBase + FIELD_OFFSET(IMAGE_TLS_DIRECTORY, 
        Characteristics), Characteristics);

      if (GetCharacteristics() != Characteristics)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("TlsDir::SetCharacteristics") << 
          ErrorString("Could not set data. Verification mismatch."));
      }
    }

    // Get list of TLS callbacks
    std::vector<PIMAGE_TLS_CALLBACK> TlsDir::GetCallbacks() const
    {
      // Callback list
      std::vector<PIMAGE_TLS_CALLBACK> Callbacks;

      // Get NT headers
      NtHeaders MyNtHeaders(m_PeFile);

      // Get pointer to callback list
      PIMAGE_TLS_CALLBACK* pCallbacks = reinterpret_cast<PIMAGE_TLS_CALLBACK*>(
        m_PeFile.RvaToVa(static_cast<DWORD>(GetAddressOfCallBacks() - 
        MyNtHeaders.GetImageBase())));

      // Loop over all callbacks
      for (PIMAGE_TLS_CALLBACK pCallback = m_Memory.Read<PIMAGE_TLS_CALLBACK>(
        pCallbacks); pCallback; pCallback = m_Memory.Read<PIMAGE_TLS_CALLBACK>(
        ++pCallbacks))
      {
        auto pCallbackRealTemp = reinterpret_cast<DWORD_PTR>(pCallback) - 
          MyNtHeaders.GetImageBase();
        auto pCallbackReal = reinterpret_cast<PIMAGE_TLS_CALLBACK>(
          pCallbackRealTemp);
        Callbacks.push_back(pCallbackReal);
      }

      // Return callback list
      return Callbacks;
    }

    // Get base of export dir
    PVOID TlsDir::GetBase() const
    {
      // Set base pointer on first request
      if (!m_pBase)
      {
        // Get NT headers
        NtHeaders const MyNtHeaders(m_PeFile);
  
        // Get export dir data
        DWORD const DataDirSize = MyNtHeaders.GetDataDirectorySize(NtHeaders::
          DataDir_TLS);
        DWORD const DataDirVa = MyNtHeaders.GetDataDirectoryVirtualAddress(
          NtHeaders::DataDir_TLS);
        if (!DataDirSize || !DataDirVa)
        {
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("TlsDir::GetBase") << 
            ErrorString("PE file has no TLS directory."));
        }
  
        // Get base of TLS dir
        m_pBase = static_cast<PBYTE>(m_PeFile.RvaToVa(DataDirVa));
      }
      
      // Return cached pointer
      return m_pBase;
    }

    // Get raw TLS dir
    IMAGE_TLS_DIRECTORY TlsDir::GetTlsDirRaw() const
    {
      // Get raw TLS dir
      return m_Memory.Read<IMAGE_TLS_DIRECTORY>(GetBase());
    }
  }
}

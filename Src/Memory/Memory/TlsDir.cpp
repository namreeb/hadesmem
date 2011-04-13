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
#include "TlsDir.hpp"
#include "PeFile.hpp"
#include "NtHeaders.hpp"
#include "MemoryMgr.hpp"

namespace Hades
{
  namespace Memory
  {
    // Constructor
    TlsDir::TlsDir(PeFile const& MyPeFile)
      : m_PeFile(MyPeFile), 
      m_Memory(m_PeFile.GetMemoryMgr())
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
      PBYTE const pTlsDir = GetBase();
      return m_Memory.Read<DWORD_PTR>(pTlsDir + FIELD_OFFSET(
        IMAGE_TLS_DIRECTORY, StartAddressOfRawData));
    }

    // Get end address of raw data
    DWORD_PTR TlsDir::GetEndAddressOfRawData() const
    {
      PBYTE const pTlsDir = GetBase();
      return m_Memory.Read<DWORD_PTR>(pTlsDir + FIELD_OFFSET(
        IMAGE_TLS_DIRECTORY, EndAddressOfRawData));
    }

    // Get address of index
    DWORD_PTR TlsDir::GetAddressOfIndex() const
    {
      PBYTE const pTlsDir = GetBase();
      return m_Memory.Read<DWORD_PTR>(pTlsDir + FIELD_OFFSET(
        IMAGE_TLS_DIRECTORY, AddressOfIndex));
    }

    // Get address of callbacks
    DWORD_PTR TlsDir::GetAddressOfCallBacks() const
    {
      PBYTE const pTlsDir = GetBase();
      return m_Memory.Read<DWORD_PTR>(pTlsDir + FIELD_OFFSET(
        IMAGE_TLS_DIRECTORY, AddressOfCallBacks));
    }

    // Get size of zero fill
    DWORD TlsDir::GetSizeOfZeroFill() const
    {
      PBYTE const pTlsDir = GetBase();
      return m_Memory.Read<DWORD>(pTlsDir + FIELD_OFFSET(
        IMAGE_TLS_DIRECTORY, SizeOfZeroFill));
    }

    // Get characteristics
    DWORD TlsDir::GetCharacteristics() const
    {
      PBYTE const pTlsDir = GetBase();
      return m_Memory.Read<DWORD>(pTlsDir + FIELD_OFFSET(
        IMAGE_TLS_DIRECTORY, Characteristics));
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
    PBYTE TlsDir::GetBase() const
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

      return static_cast<PBYTE>(m_PeFile.RvaToVa(DataDirVa));
    }

    // Get raw TLS dir
    IMAGE_TLS_DIRECTORY TlsDir::GetTlsDirRaw() const
    {
      // Get raw TLS dir
      return m_Memory.Read<IMAGE_TLS_DIRECTORY>(GetBase());
    }
  }
}

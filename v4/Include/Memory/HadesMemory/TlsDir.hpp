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

#pragma once

// C++ Standard Library
#include <vector>

// Windows
#include <Windows.h>

// Hades
#include "Fwd.hpp"
#include "Error.hpp"
#include "PeFile.hpp"
#include "MemoryMgr.hpp"

namespace Hades
{
  namespace Memory
  {
    // PE file TLS directory
    class TlsDir
    {
    public:
      // TlsDir error class
      class Error : public virtual HadesMemError
      { };

      // Constructor
      explicit TlsDir(PeFile const& MyPeFile);

      // Whether TLS directory is valid
      bool IsValid() const;

      // Ensure TLS directory is valid
      void EnsureValid() const;

      // Get start address of raw data
      DWORD_PTR GetStartAddressOfRawData() const;

      // Set start address of raw data
      void SetStartAddressOfRawData(DWORD_PTR StartAddressOfRawData) const;

      // Get end address of raw data
      DWORD_PTR GetEndAddressOfRawData() const;

      // Set end address of raw data
      void SetEndAddressOfRawData(DWORD_PTR EndAddressOfRawData) const;

      // Get address of index
      DWORD_PTR GetAddressOfIndex() const;

      // Set address of index
      void SetAddressOfIndex(DWORD_PTR AddressOfIndex) const;

      // Get address of callbacks
      DWORD_PTR GetAddressOfCallBacks() const;

      // Set address of callbacks
      void SetAddressOfCallBacks(DWORD_PTR AddressOfCallbacks) const;

      // Get size of zero fill
      DWORD GetSizeOfZeroFill() const;

      // Set size of zero fill
      void SetSizeOfZeroFill(DWORD SizeOfZeroFill) const;

      // Get characteristics
      DWORD GetCharacteristics() const;

      // Set characteristics
      void SetCharacteristics(DWORD Characteristics) const;

      // Get list of TLS callbacks
      std::vector<PIMAGE_TLS_CALLBACK> GetCallbacks() const;
        
      // Todo: SetCallbacks function

      // Get base of TLS dir
      PBYTE GetBase() const;

      // Get raw TLS dir
      IMAGE_TLS_DIRECTORY GetTlsDirRaw() const;

    private:
      // PE file
      PeFile m_PeFile;

      // Memory instance
      MemoryMgr m_Memory;
    };
  }
}

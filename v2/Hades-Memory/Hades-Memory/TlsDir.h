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

#pragma once

// Windows
#include <Windows.h>

// C++ Standard Library
#include <vector>

// Boost
#pragma warning(push, 1)
#pragma warning (disable: ALL_CODE_ANALYSIS_WARNINGS)
#include <boost/noncopyable.hpp>
#pragma warning(pop)

// Hades
#include "Fwd.h"
#include "Error.h"
#include "PeFile.h"
#include "MemoryMgr.h"

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
      TlsDir(PeFile const& MyPeFile);

      // Whether TLS directory is valid
      bool IsValid() const;

      // Ensure TLS directory is valid
      void EnsureValid() const;

      // Get start address of raw data
      DWORD_PTR GetStartAddressOfRawData() const;

      // Get end address of raw data
      DWORD_PTR GetEndAddressOfRawData() const;

      // Get address of index
      DWORD_PTR GetAddressOfIndex() const;

      // Get address of callbacks
      DWORD_PTR GetAddressOfCallBacks() const;

      // Get size of zero fill
      DWORD GetSizeOfZeroFill() const;

      // Get characteristics
      DWORD GetCharacteristics() const;

      // Get list of TLS callbacks
      std::vector<PIMAGE_TLS_CALLBACK> GetCallbacks() const;

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

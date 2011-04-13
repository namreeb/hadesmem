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
#include <string>

// Boost
#pragma warning(push, 1)
#include <boost/iterator/iterator_facade.hpp>
#pragma warning(pop)

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
    // Import directory wrapper
    class ImportDir
    {
    public:
      // ImportDir error type
      class Error : public virtual HadesMemError
      { };

      // Constructor
      explicit ImportDir(PeFile const& MyPeFile, 
        PIMAGE_IMPORT_DESCRIPTOR pImpDesc = nullptr);

      // Whether import directory is valid
      bool IsValid() const;

      // Ensure import directory is valid
      void EnsureValid() const;

      // Get import directory base
      PBYTE GetBase() const;

      // Advance to next descriptor
      void Advance() const;

      // Get characteristics
      DWORD GetCharacteristics() const;

      // Get time and date stamp
      DWORD GetTimeDateStamp() const;

      // Get forwarder chain
      DWORD GetForwarderChain() const;

      // Get name (raw)
      DWORD GetNameRaw() const;

      // Get name
      std::string GetName() const;

      // Get first think
      DWORD GetFirstThunk() const;

    private:
      PeFile m_PeFile;

      MemoryMgr m_Memory;

      mutable PIMAGE_IMPORT_DESCRIPTOR m_pImpDesc;
    };

    // Import thunk wrapper
    class ImportThunk
    {
    public:
      // ImportDir error type
      class Error : public virtual HadesMemError
      { };

      // Constructor
      ImportThunk(PeFile const& MyPeFile, PVOID pThunk);

      // Whether thunk is valid
      bool IsValid() const;

      // Advance to next thunk
      void Advance() const;

      // Get address of data
      DWORD_PTR GetAddressOfData() const;

      // Get ordinal (raw)
      DWORD_PTR GetOrdinalRaw() const;
      
      // Whether import is by ordinal
      bool ByOrdinal() const;

      // Get ordinal
      WORD GetOrdinal() const;

      // Get function
      DWORD_PTR GetFunction() const;

      // Get hint
      WORD GetHint() const;

      // Get name
      std::string GetName() const;

      // Set function
      void SetFunction(DWORD_PTR Function) const;

    private:
      PeFile m_PeFile;

      MemoryMgr m_Memory;

      mutable PIMAGE_THUNK_DATA m_pThunk;

      mutable PBYTE m_pBase;
    };
  }
}

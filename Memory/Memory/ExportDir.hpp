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
#include <string>
#include <utility>

// Boost
#ifdef _MSC_VER
#pragma warning(push, 1)
#endif // #ifdef _MSC_VER
#include <boost/noncopyable.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif // #ifdef _MSC_VER

// Hades
#include "Fwd.hpp"
#include "Error.hpp"
#include "PeFile.hpp"
#include "MemoryMgr.hpp"

namespace Hades
{
  namespace Memory
  {
    // PE file export directory
    class ExportDir
    {
    public:
      // ExportDir error class
      class Error : public virtual HadesMemError
      { };

      // Constructor
      ExportDir(PeFile const& MyPeFile);

      // Whether export directory is valid
      bool IsValid() const;

      // Ensure export directory is valid
      void EnsureValid() const;

      // Get characteristics
      DWORD GetCharacteristics() const;

      // Get time date stamp
      DWORD GetTimeDateStamp() const;

      // Get major version
      WORD GetMajorVersion() const;

      // Get minor version
      WORD GetMinorVersion() const;

      // Get module name
      std::string GetName() const;

      // Get ordinal base
      DWORD GetOrdinalBase() const;

      // Get number of functions
      DWORD GetNumberOfFunctions() const;

      // Get number of names
      DWORD GetNumberOfNames() const;

      // Get address of functions
      DWORD GetAddressOfFunctions() const;

      // Get address of names
      DWORD GetAddressOfNames() const;

      // Get address of name ordinals
      DWORD GetAddressOfNameOrdinals() const;

      // Get base of export dir
      PBYTE GetBase() const;

      // Get raw export dir
      IMAGE_EXPORT_DIRECTORY GetExportDirRaw() const;

    private:
      // PE file
      PeFile m_PeFile;

      // Memory instance
      MemoryMgr m_Memory;

      // Base of export dir
      mutable PBYTE m_pBase;
    };

    // PE file export data
    class Export
    {
    public:
      // Constructor
      Export(PeFile const& MyPeFile, DWORD Ordinal);

      // Get RVA
      DWORD GetRva() const
      {
        return m_Rva;
      }

      // Get VA
      PVOID GetVa() const
      {
        return m_Va;
      }

      // Get name
      std::string GetName() const
      {
        return m_Name;
      }

      // Get forwarder
      std::string GetForwarder() const
      {
        return m_Forwarder;
      }
      
      // Get forwarder module name
      std::string GetForwarderModule() const
      {
        return m_ForwarderSplit.first;
      }
      
      // Get forwarder function name
      std::string GetForwarderFunction() const
      {
        return m_ForwarderSplit.second;
      }

      // Get ordinal
      WORD GetOrdinal() const
      {
        return m_Ordinal;
      }

      // If entry is exported by name
      bool ByName() const
      {
        return m_ByName;
      }

      // If entry is forwarded
      bool Forwarded() const
      {
        return m_Forwarded;
      }

    private:
      // PE file instance
      PeFile m_PeFile;
      // Memory instance
      MemoryMgr m_Memory;

      // RVA
      DWORD m_Rva;
      // VA
      PVOID m_Va;
      // Name
      std::string m_Name;
      // Forwarder
      std::string m_Forwarder;
      // Split forwarder
      std::pair<std::string, std::string> m_ForwarderSplit;
      // Ordinal
      WORD m_Ordinal;
      // If entry is exported by name
      bool m_ByName;
      // If entry is forwarded
      bool m_Forwarded;
    };
  }
}

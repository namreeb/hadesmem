/*
This file is part of HadesMem.
Copyright (C) 2010 Joshua Boyce (aka RaptorFactor, Cypherjb, Cypher, Chazwazza).
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
#include <utility>

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
    // PE file export directory
    class ExportDir
    {
    public:
      // ExportDir error class
      class Error : public virtual HadesMemError
      { };

      // Constructor
      explicit ExportDir(PeFile const& MyPeFile);

      // Whether export directory is valid
      bool IsValid() const;

      // Ensure export directory is valid
      void EnsureValid() const;

      // Get characteristics
      DWORD GetCharacteristics() const;

      // Set characteristics
      void SetCharacteristics(DWORD Characteristics) const;

      // Get time date stamp
      DWORD GetTimeDateStamp() const;

      // Set time date stamp
      void SetTimeDateStamp(DWORD TimeDateStamp) const;

      // Get major version
      WORD GetMajorVersion() const;

      // Set major version
      void SetMajorVersion(WORD MajorVersion) const;

      // Get minor version
      WORD GetMinorVersion() const;

      // Set minor version
      void SetMinorVersion(WORD MinorVersion) const;

      // Get module name
      std::string GetName() const;

      // Get ordinal base
      DWORD GetOrdinalBase() const;

      // Set ordinal base
      void SetOrdinalBase(DWORD OrdinalBase) const;

      // Get number of functions
      DWORD GetNumberOfFunctions() const;

      // Set number of functions
      void SetNumberOfFunctions(DWORD NumberOfFunctions) const;

      // Get number of names
      DWORD GetNumberOfNames() const;

      // Set number of names
      void SetNumberOfNames(DWORD NumberOfNames) const;

      // Get address of functions
      DWORD GetAddressOfFunctions() const;

      // Set address of functions
      void SetAddressOfFunctions(DWORD AddressOfFunctions) const;

      // Get address of names
      DWORD GetAddressOfNames() const;

      // Set address of names
      void SetAddressOfNames(DWORD AddressOfNames) const;

      // Get address of name ordinals
      DWORD GetAddressOfNameOrdinals() const;

      // Set address of name ordinals
      void SetAddressOfNameOrdinals(DWORD AddressOfNameOrdinals) const;

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
      DWORD GetRva() const;

      // Get VA
      PVOID GetVa() const;

      // Get name
      std::string GetName() const;

      // Get forwarder
      std::string GetForwarder() const;
      
      // Get forwarder module name
      std::string GetForwarderModule() const;
      
      // Get forwarder function name
      std::string GetForwarderFunction() const;

      // Get ordinal
      WORD GetOrdinal() const;

      // If entry is exported by name
      bool ByName() const;

      // If entry is forwarded
      bool Forwarded() const;

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

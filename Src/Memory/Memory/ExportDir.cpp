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
#include <vector>

// Hades
#include "PeFile.hpp"
#include "NtHeaders.hpp"
#include "DosHeader.hpp"
#include "MemoryMgr.hpp"
#include "ExportDir.hpp"

namespace Hades
{
  namespace Memory
  {
    // Constructor
    ExportDir::ExportDir(PeFile const& MyPeFile)
      : m_PeFile(MyPeFile), 
      m_Memory(m_PeFile.GetMemoryMgr()), 
      m_pBase(nullptr)
    { }

    // Whether export directory is valid
    bool ExportDir::IsValid() const
    {
      // Get NT headers
      NtHeaders const MyNtHeaders(m_PeFile);

      // Get export dir data
      DWORD const DataDirSize(MyNtHeaders.GetDataDirectorySize(NtHeaders::
        DataDir_Export));
      DWORD const DataDirVa(MyNtHeaders.GetDataDirectoryVirtualAddress(
        NtHeaders::DataDir_Export));

      // Export dir is valid if size and rva are valid
      return DataDirSize && DataDirVa;
    }

    // Ensure export directory is valid
    void ExportDir::EnsureValid() const
    {
      if (!IsValid())
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("ExportDir::EnsureValid") << 
          ErrorString("Export directory is invalid."));
      }
    }

    // Get characteristics
    DWORD ExportDir::GetCharacteristics() const
    {
      PBYTE const pExportDir = GetBase();
      return m_Memory.Read<DWORD>(pExportDir + FIELD_OFFSET(
        IMAGE_EXPORT_DIRECTORY, Characteristics));
    }

    // Get time date stamp
    DWORD ExportDir::GetTimeDateStamp() const
    {
      PBYTE const pExportDir = GetBase();
      return m_Memory.Read<DWORD>(pExportDir + FIELD_OFFSET(
        IMAGE_EXPORT_DIRECTORY, TimeDateStamp));
    }

    // Get major version
    WORD ExportDir::GetMajorVersion() const
    {
      PBYTE const pExportDir = GetBase();
      return m_Memory.Read<WORD>(pExportDir + FIELD_OFFSET(
        IMAGE_EXPORT_DIRECTORY, MajorVersion));
    }

    // Get minor version
    WORD ExportDir::GetMinorVersion() const
    {
      PBYTE const pExportDir = GetBase();
      return m_Memory.Read<WORD>(pExportDir + FIELD_OFFSET(
        IMAGE_EXPORT_DIRECTORY, MinorVersion));
    }

    // Get module name
    std::string ExportDir::GetName() const
    {
      // Get base of export dir
      PBYTE const pExpDirBase = GetBase();

      // Read RVA of module name
      DWORD const NameRva = m_Memory.Read<DWORD>(pExpDirBase + FIELD_OFFSET(
        IMAGE_EXPORT_DIRECTORY, Name));

      // Ensure there is a module name to process
      if (!NameRva)
      {
        return std::string();
      }

      // Read module name
      return m_Memory.Read<std::string>(m_PeFile.RvaToVa(NameRva));
    }

    // Get ordinal base
    DWORD ExportDir::GetOrdinalBase() const
    {
      PBYTE const pExportDir = GetBase();
      return m_Memory.Read<DWORD>(pExportDir + FIELD_OFFSET(
        IMAGE_EXPORT_DIRECTORY, Base));
    }

    // Get number of functions
    DWORD ExportDir::GetNumberOfFunctions() const
    {
      PBYTE const pExportDir = GetBase();
      return m_Memory.Read<DWORD>(pExportDir + FIELD_OFFSET(
        IMAGE_EXPORT_DIRECTORY, NumberOfFunctions));
    }

    // Get number of names
    DWORD ExportDir::GetNumberOfNames() const
    {
      PBYTE const pExportDir = GetBase();
      return m_Memory.Read<DWORD>(pExportDir + FIELD_OFFSET(
        IMAGE_EXPORT_DIRECTORY, NumberOfNames));
    }

    // Get address of functions
    DWORD ExportDir::GetAddressOfFunctions() const
    {
      PBYTE const pExportDir = GetBase();
      return m_Memory.Read<DWORD>(pExportDir + FIELD_OFFSET(
        IMAGE_EXPORT_DIRECTORY, AddressOfFunctions));
    }

    // Get address of names
    DWORD ExportDir::GetAddressOfNames() const
    {
      PBYTE const pExportDir = GetBase();
      return m_Memory.Read<DWORD>(pExportDir + FIELD_OFFSET(
        IMAGE_EXPORT_DIRECTORY, AddressOfNames));
    }

    // Get address of name ordinals
    DWORD ExportDir::GetAddressOfNameOrdinals() const
    {
      PBYTE const pExportDir = GetBase();
      return m_Memory.Read<DWORD>(pExportDir + FIELD_OFFSET(
        IMAGE_EXPORT_DIRECTORY, AddressOfNameOrdinals));
    }

    // Get base of export dir
    PBYTE ExportDir::GetBase() const
    {
      // Initialize base address if necessary
      if (!m_pBase)
      {
        // Get NT headers
        NtHeaders const MyNtHeaders(m_PeFile);

        // Get export dir data
        DWORD const DataDirSize = MyNtHeaders.GetDataDirectorySize(NtHeaders::
          DataDir_Export);
        DWORD const DataDirVa = MyNtHeaders.GetDataDirectoryVirtualAddress(
          NtHeaders::DataDir_Export);
        if (!DataDirSize || !DataDirVa)
        {
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("ExportDir::GetBase") << 
            ErrorString("PE file has no export directory."));
        }

        // Init base address
        m_pBase = static_cast<PBYTE>(m_PeFile.RvaToVa(DataDirVa));
      }

      // Return base address
      return m_pBase;
    }

    // Get raw export dir
    IMAGE_EXPORT_DIRECTORY ExportDir::GetExportDirRaw() const
    {
      // Get raw export dir
      return m_Memory.Read<IMAGE_EXPORT_DIRECTORY>(GetBase());
    }

    // Constructor
    Export::Export(PeFile const& MyPeFile, DWORD Ordinal) 
      : m_PeFile(MyPeFile), 
      m_Memory(MyPeFile.GetMemoryMgr()), 
      m_Rva(0), 
      m_Va(nullptr), 
      m_Name(), 
      m_Forwarder(), 
      m_ForwarderSplit(), 
      m_Ordinal(0), 
      m_ByName(false), 
      m_Forwarded(false)
    {
      // Get NT headers
      NtHeaders const MyNtHeaders(m_PeFile);

      // Get export directory
      ExportDir const MyExportDir(m_PeFile);

      // Current function offset
      DWORD Offset = Ordinal - MyExportDir.GetOrdinalBase();

      // Ensure export number is valid
      if (Offset > MyExportDir.GetNumberOfFunctions())
      {
        BOOST_THROW_EXCEPTION(ExportDir::Error() << 
          ErrorFunction("Export::Export") << 
          ErrorString("Invalid export number."));
      }

      // Get pointer to function name ordinals
      WORD* pOrdinals = static_cast<WORD*>(m_PeFile.RvaToVa(MyExportDir.
        GetAddressOfNameOrdinals()));
      // Get pointer to functions
      DWORD* pFunctions = static_cast<DWORD*>(m_PeFile.RvaToVa(MyExportDir.
        GetAddressOfFunctions()));
      // Get pointer to function names
      DWORD* pNames = static_cast<DWORD*>(m_PeFile.RvaToVa(MyExportDir.
        GetAddressOfNames()));

      // Get data directory size
      DWORD const DataDirSize = MyNtHeaders.GetDataDirectorySize(NtHeaders::
        DataDir_Export);
      // Get data directory VA
      DWORD const DataDirVa = MyNtHeaders.GetDataDirectoryVirtualAddress(
        NtHeaders::DataDir_Export);

      // Get start of export dir
      DWORD const ExportDirStart = DataDirVa;
      // Get end of export dir
      DWORD const ExportDirEnd = ExportDirStart + DataDirSize;

      // Find next exported entry
      for (; !m_Memory.Read<DWORD>(pFunctions + Offset) && 
        Offset <= MyExportDir.GetNumberOfFunctions(); ++Offset)
        ;

      // Ensure export number is valid
      if (Offset > MyExportDir.GetNumberOfFunctions())
      {
        BOOST_THROW_EXCEPTION(ExportDir::Error() << 
          ErrorFunction("Export::Export") << 
          ErrorString("Invalid export number."));
      }

      // Set new ordinal
      Ordinal = Offset + MyExportDir.GetOrdinalBase();

      // Set ordinal
      m_Ordinal = static_cast<WORD>(Ordinal);

      // Find ordinal name and set (if applicable)
      if (DWORD const NumberOfNames = MyExportDir.GetNumberOfNames())
      {
        std::vector<WORD> NameOrdinals(m_Memory.Read<std::vector<WORD>>(
          pOrdinals, NumberOfNames));
        auto NameOrdIter = std::find(NameOrdinals.begin(), NameOrdinals.end(), 
          Offset);
        if (NameOrdIter != NameOrdinals.end())
        {
          m_ByName = true;
          DWORD const NameRva = m_Memory.Read<DWORD>(pNames + std::distance(
            NameOrdinals.begin(), NameOrdIter));
          m_Name = m_Memory.Read<std::string>(m_PeFile.RvaToVa(NameRva));
        }
      }

      // Get function RVA (unchecked)
      DWORD const FuncRva = m_Memory.Read<DWORD>(pFunctions + Offset);

      // Check function RVA. If it lies inside the export dir region 
      // then it's a forwarded export. Otherwise it's a regular RVA.
      if (FuncRva > ExportDirStart && FuncRva < ExportDirEnd)
      {
        // Set export forwarder
        m_Forwarded = true;
        m_Forwarder = m_Memory.Read<std::string>(m_PeFile.RvaToVa(FuncRva));
          
        // Split forwarder
        std::string::size_type SplitPos = m_Forwarder.rfind('.');
        if (SplitPos != std::string::npos)
        {
          m_ForwarderSplit = std::make_pair(m_Forwarder.substr(0, SplitPos), 
            m_Forwarder.substr(SplitPos));
        }
        else
        {
          BOOST_THROW_EXCEPTION(ExportDir::Error() << 
            ErrorFunction("Export::Export") << 
            ErrorString("Invalid forwarder string format."));
        }
      }
      else
      {
        // Set export RVA/VA
        m_Rva = FuncRva;
        m_Va = m_PeFile.RvaToVa(FuncRva);
      }
    }
  }
}

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

// Hades
#include "Fwd.hpp"
#include "Error.hpp"
#include "PeFile.hpp"
#include "MemoryMgr.hpp"

namespace Hades
{
  namespace Memory
  {
    // PE file NT headers
    class NtHeaders
    {
    public:
      // NT headers error class
      class Error : public virtual HadesMemError
      { };

      // Data directory entries
      enum DataDir
      {
        DataDir_Export, 
        DataDir_Import, 
        DataDir_Resource, 
        DataDir_Exception, 
        DataDir_Security, 
        DataDir_BaseReloc, 
        DataDir_Debug, 
        DataDir_Architecture, 
        DataDir_GlobalPTR, 
        DataDir_TLS, 
        DataDir_LoadConfig, 
        DataDir_BoundImport, 
        DataDir_IAT, 
        DataDir_DelayImport, 
        DataDir_COMDescriptor
      };

      // Constructor
      explicit NtHeaders(PeFile const& MyPeFile);

      // Get base of NT headers
      PBYTE GetBase() const;

      // Whether signature is valid
      bool IsSignatureValid() const;

      // Ensure signature is valid
      void EnsureSignatureValid() const;

      // Get signature
      DWORD GetSignature() const;

      // Set signature
      void SetSignature(DWORD Signature) const;

      // Get machine
      WORD GetMachine() const;

      // Set machine
      void SetMachine(WORD Machine) const;

      // Get number of sections
      WORD GetNumberOfSections() const;

      // Set number of sections
      void SetNumberOfSections(WORD NumberOfSections) const;

      // Get time date stamp
      DWORD GetTimeDateStamp() const;

      // Set time date stamp
      void SetTimeDateStamp(DWORD TimeDateStamp) const;

      // Get pointer to symbol table
      DWORD GetPointerToSymbolTable() const;

      // Set pointer to symbol table
      void SetPointerToSymbolTable(DWORD PointerToSymbolTable) const;

      // Get number of symbols
      DWORD GetNumberOfSymbols() const;

      // Set number of symbols
      void SetNumberOfSymbols(DWORD NumberOfSymbols) const;

      // Get size of optional header
      WORD GetSizeOfOptionalHeader() const;

      // Set size of optional header
      void SetSizeOfOptionalHeader(WORD SizeOfOptionalHeader) const;

      // Get characteristics
      WORD GetCharacteristics() const;

      // Set characteristics
      void SetCharacteristics(WORD Characteristics) const;

      // Get magic
      WORD GetMagic() const;

      // Set magic
      void SetMagic(WORD Magic) const;

      // Get major linker version
      BYTE GetMajorLinkerVersion() const;

      // Set major linker version
      void SetMajorLinkerVersion(BYTE MajorLinkerVersion) const;

      // Get minor linker version
      BYTE GetMinorLinkerVersion() const;

      // Set major linker version
      void SetMinorLinkerVersion(BYTE MinorLinkerVersion) const;

      // Get minor linker version
      DWORD GetSizeOfCode() const;

      // Set major linker version
      void SetSizeOfCode(DWORD SizeOfCode) const;

      // Get minor linker version
      DWORD GetSizeOfInitializedData() const;

      // Set major linker version
      void SetSizeOfInitializedData(DWORD SizeOfInitializedData) const;

      // Get minor linker version
      DWORD GetSizeOfUninitializedData() const;

      // Set major linker version
      void SetSizeOfUninitializedData(DWORD SizeOfUninitializedData) const;

      // Get minor linker version
      DWORD GetAddressOfEntryPoint() const;

      // Set major linker version
      void SetAddressOfEntryPoint(DWORD AddressOfEntryPoint) const;

      // Get base of code
      DWORD GetBaseOfCode() const;

      // Set base of code
      void SetBaseOfCode(DWORD BaseOfCode) const;

#if defined(_M_IX86) 
      // Get base of data
      DWORD GetBaseOfData() const;

      // Set base of data
      void SetBaseOfData(DWORD BaseOfData) const;
#endif

      // Get base of code
      ULONG_PTR GetImageBase() const;

      // Set base of code
      void SetImageBase(ULONG_PTR ImageBase) const;

      // Get base of code
      DWORD GetSectionAlignment() const;

      // Set base of code
      void SetSectionAlignment(DWORD SectionAlignment) const;

      // Get base of code
      DWORD GetFileAlignment() const;

      // Set base of code
      void SetFileAlignment(DWORD FileAlignment) const;

      // Get base of code
      WORD GetMajorOperatingSystemVersion() const;

      // Set base of code
      void SetMajorOperatingSystemVersion(
        WORD MajorOperatingSystemVersion) const;

      // Get base of code
      WORD GetMinorOperatingSystemVersion() const;

      // Set base of code
      void SetMinorOperatingSystemVersion(
        WORD MinorOperatingSystemVersion) const;

      // Get base of code
      WORD GetMajorImageVersion() const;

      // Set base of code
      void SetMajorImageVersion(WORD MajorImageVersion) const;

      // Get base of code
      WORD GetMinorImageVersion() const;

      // Set base of code
      void SetMinorImageVersion(WORD MinorImageVersion) const;

      // Get base of code
      WORD GetMajorSubsystemVersion() const;

      // Set base of code
      void SetMajorSubsystemVersion(WORD MajorSubsystemVersion) const;

      // Get base of code
      WORD GetMinorSubsystemVersion() const;

      // Set base of code
      void SetMinorSubsystemVersion(WORD MinorSubsystemVersion) const;

      // Get base of code
      DWORD GetWin32VersionValue() const;

      // Set base of code
      void SetWin32VersionValue(DWORD Win32VersionValue) const;

      // Get size of image
      DWORD GetSizeOfImage() const;
      
      // Set size of image
      void SetSizeOfImage(DWORD SizeOfImage) const;

      // Get base of code
      DWORD GetSizeOfHeaders() const;

      // Set base of code
      void SetSizeOfHeaders(DWORD SizeOfHeaders) const;

      // Get base of code
      DWORD GetCheckSum() const;

      // Set base of code
      void SetCheckSum(DWORD CheckSum) const;

      // Get base of code
      WORD GetSubsystem() const;

      // Set base of code
      void SetSubsystem(WORD Subsystem) const;

      // Get base of code
      WORD GetDllCharacteristics() const;

      // Set base of code
      void SetDllCharacteristics(WORD DllCharacteristics) const;

      // Get base of code
      ULONG_PTR GetSizeOfStackReserve() const;

      // Set base of code
      void SetSizeOfStackReserve(ULONG_PTR SizeOfStackReserve) const;

      // Get base of code
      ULONG_PTR GetSizeOfStackCommit() const;

      // Set base of code
      void SetSizeOfStackCommit(ULONG_PTR SizeOfStackCommit) const;

      // Get base of code
      ULONG_PTR GetSizeOfHeapReserve() const;

      // Set base of code
      void SetSizeOfHeapReserve(ULONG_PTR SizeOfHeapReserve) const;

      // Get base of code
      ULONG_PTR GetSizeOfHeapCommit() const;

      // Set base of code
      void SetSizeOfHeapCommit(ULONG_PTR SizeOfHeapCommit) const;

      // Get base of code
      DWORD GetLoaderFlags() const;

      // Set base of code
      void SetLoaderFlags(DWORD LoaderFlags) const;

      // Get base of code
      DWORD GetNumberOfRvaAndSizes() const;

      // Set base of code
      void SetNumberOfRvaAndSizes(DWORD NumberOfRvaAndSizes) const;

      // Get base of code
      DWORD GetDataDirectoryVirtualAddress(DataDir MyDataDir) const;

      // Set base of code
      void SetDataDirectoryVirtualAddress(DataDir MyDataDir, 
        DWORD DataDirectoryVirtualAddress) const;

      // Get base of code
      DWORD GetDataDirectorySize(DataDir MyDataDir) const;

      // Set base of code
      void SetDataDirectorySize(DataDir MyDataDir, 
        DWORD DataDirectorySize) const;

      // Get raw NT headers
      IMAGE_NT_HEADERS GetHeadersRaw() const;

    private:
      // PE file
      PeFile m_PeFile;

      // Memory instance
      MemoryMgr m_Memory;

      // Base address
      mutable PBYTE m_pBase;
    };
  }
}

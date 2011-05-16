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
#include "PeFile.hpp"
#include "MemoryMgr.hpp"
#include "DosHeader.hpp"
#include "NtHeaders.hpp"

namespace Hades
{
  namespace Memory
  {
    // Constructor
    NtHeaders::NtHeaders(PeFile const& MyPeFile)
      : m_PeFile(MyPeFile), 
      m_Memory(m_PeFile.GetMemoryMgr()), 
      m_pBase(nullptr)
    {
      // Get base
      DosHeader MyDosHeader(m_PeFile);
      m_pBase = static_cast<PBYTE>(m_PeFile.GetBase()) + 
        MyDosHeader.GetNewHeaderOffset();
      
      // Ensure signature is valid
      EnsureSignatureValid();
    }

    // Get base of NT headers
    PVOID NtHeaders::GetBase() const
    {
      // Return base address
      return m_pBase;
    }

    // Whether signature is valid
    bool NtHeaders::IsSignatureValid() const
    {
      return IMAGE_NT_SIGNATURE == GetSignature();
    }

    // Ensure signature is valid
    void NtHeaders::EnsureSignatureValid() const
    {
      if (!IsSignatureValid())
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("NtHeaders::EnsureSignatureValid") << 
          ErrorString("NT headers signature invalid."));
      }
    }

    // Get signature
    DWORD NtHeaders::GetSignature() const
    {
      return m_Memory.Read<DWORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        Signature));
    }

    // Set signature
    void NtHeaders::SetSignature(DWORD Signature) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, Signature), 
        Signature);
    }

    // Get machine
    WORD NtHeaders::GetMachine() const
    {
      return m_Memory.Read<WORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        FileHeader.Machine));
    }

    // Set machine
    void NtHeaders::SetMachine(WORD Machine) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, FileHeader.
        Machine), Machine);
    }

    // Get number of sections
    WORD NtHeaders::GetNumberOfSections() const
    {
      return m_Memory.Read<WORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        FileHeader.NumberOfSections));
    }

    // Set number of sections
    void NtHeaders::SetNumberOfSections(WORD NumberOfSections) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, FileHeader.
        NumberOfSections), NumberOfSections);
    }

    // Get time date stamp
    DWORD NtHeaders::GetTimeDateStamp() const
    {
      return m_Memory.Read<DWORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        FileHeader.TimeDateStamp));
    }

    // Set time date stamp
    void NtHeaders::SetTimeDateStamp(DWORD TimeDateStamp) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, FileHeader.
        TimeDateStamp), TimeDateStamp);
    }

    // Get pointer to symbol table
    DWORD NtHeaders::GetPointerToSymbolTable() const
    {
      return m_Memory.Read<DWORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        FileHeader.PointerToSymbolTable));
    }

    // Set pointer to symbol table
    void NtHeaders::SetPointerToSymbolTable(DWORD PointerToSymbolTable) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, FileHeader.
        PointerToSymbolTable), PointerToSymbolTable);
    }

    // Get number of symbols
    DWORD NtHeaders::GetNumberOfSymbols() const
    {
      return m_Memory.Read<DWORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        FileHeader.NumberOfSymbols));
    }

    // Set number of symbols
    void NtHeaders::SetNumberOfSymbols(DWORD NumberOfSymbols) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, FileHeader.
        NumberOfSymbols), NumberOfSymbols);
    }

    // Get size of optional header
    WORD NtHeaders::GetSizeOfOptionalHeader() const
    {
      return m_Memory.Read<WORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        FileHeader.SizeOfOptionalHeader));
    }

    // Set size of optional header
    void NtHeaders::SetSizeOfOptionalHeader(WORD SizeOfOptionalHeader) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, FileHeader.
        SizeOfOptionalHeader), SizeOfOptionalHeader);

    }

    // Get characteristics
    WORD NtHeaders::GetCharacteristics() const
    {
      return m_Memory.Read<WORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        FileHeader.Characteristics));
    }

    // Set characteristics
    void NtHeaders::SetCharacteristics(WORD Characteristics) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, FileHeader.
        Characteristics), Characteristics);
    }

    // Get magic
    WORD NtHeaders::GetMagic() const
    {
      return m_Memory.Read<WORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.Magic));
    }

    // Set magic
    void NtHeaders::SetMagic(WORD Magic) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.Magic), Magic);
    }

    // Get major linker version
    BYTE NtHeaders::GetMajorLinkerVersion() const
    {
      return m_Memory.Read<BYTE>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.MajorLinkerVersion));
    }

    // Set major linker version
    void NtHeaders::SetMajorLinkerVersion(BYTE MajorLinkerVersion) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.MajorLinkerVersion), MajorLinkerVersion);
    }

    // Get minor linker version
    BYTE NtHeaders::GetMinorLinkerVersion() const
    {
      return m_Memory.Read<BYTE>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.MinorLinkerVersion));
    }

    // Set minor linker version
    void NtHeaders::SetMinorLinkerVersion(BYTE MinorLinkerVersion) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.MinorLinkerVersion), MinorLinkerVersion);
    }

    // Get size of code
    DWORD NtHeaders::GetSizeOfCode() const
    {
      return m_Memory.Read<DWORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.SizeOfCode));
    }

    // Set size of code
    void NtHeaders::SetSizeOfCode(DWORD SizeOfCode) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.SizeOfCode), SizeOfCode);
    }

    // Get size of initialized data
    DWORD NtHeaders::GetSizeOfInitializedData() const
    {
      return m_Memory.Read<DWORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.SizeOfInitializedData));
    }

    // Set size of initialized data
    void NtHeaders::SetSizeOfInitializedData(DWORD SizeOfInitializedData) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.SizeOfInitializedData), SizeOfInitializedData);
    }

    // Get size of uninitialized data
    DWORD NtHeaders::GetSizeOfUninitializedData() const
    {
      return m_Memory.Read<DWORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.SizeOfUninitializedData));
    }

    // Set size of uninitialized data
    void NtHeaders::SetSizeOfUninitializedData(DWORD SizeOfUninitializedData) 
      const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.SizeOfUninitializedData), SizeOfUninitializedData);
    }

    // Get address of entry point
    DWORD NtHeaders::GetAddressOfEntryPoint() const
    {
      return m_Memory.Read<DWORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.AddressOfEntryPoint));
    }

    // Set address of entry point
    void NtHeaders::SetAddressOfEntryPoint(DWORD AddressOfEntryPoint) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.AddressOfEntryPoint), AddressOfEntryPoint);
    }

    // Get base of code
    DWORD NtHeaders::GetBaseOfCode() const
    {
      return m_Memory.Read<DWORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.BaseOfCode));
    }

    // Set base of code
    void NtHeaders::SetBaseOfCode(DWORD BaseOfCode) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.BaseOfCode), BaseOfCode);
    }

#if defined(_M_IX86) 
    // Get base of data
    DWORD NtHeaders::GetBaseOfData() const
    {
      return m_Memory.Read<DWORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.BaseOfData));
    }

    // Set base of data
    void NtHeaders::SetBaseOfData(DWORD BaseOfData) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.BaseOfData), BaseOfData);
    }
#endif

    // Get image base
    ULONG_PTR NtHeaders::GetImageBase() const
    {
      return m_Memory.Read<ULONG_PTR>(m_pBase + FIELD_OFFSET(
        IMAGE_NT_HEADERS, OptionalHeader.ImageBase));
    }

    // Set image base
    void NtHeaders::SetImageBase(ULONG_PTR ImageBase) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.ImageBase), ImageBase);
    }

    // Get section alignment
    DWORD NtHeaders::GetSectionAlignment() const
    {
      return m_Memory.Read<DWORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.SectionAlignment));
    }

    // Set section alignment
    void NtHeaders::SetSectionAlignment(DWORD SectionAlignment) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.SectionAlignment), SectionAlignment);
    }

    // Get file alignment
    DWORD NtHeaders::GetFileAlignment() const
    {
      return m_Memory.Read<DWORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.FileAlignment));
    }

    // Set file alignment
    void NtHeaders::SetFileAlignment(DWORD FileAlignment) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.FileAlignment), FileAlignment);
    }

    // Get major operating system version
    WORD NtHeaders::GetMajorOperatingSystemVersion() const
    {
      return m_Memory.Read<WORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.MajorOperatingSystemVersion));
    }

    // Set major operating system version
    void NtHeaders::SetMajorOperatingSystemVersion(
      WORD MajorOperatingSystemVersion) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.MajorOperatingSystemVersion), 
        MajorOperatingSystemVersion);
    }

    // Get minor operating system version
    WORD NtHeaders::GetMinorOperatingSystemVersion() const
    {
      return m_Memory.Read<WORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.MinorOperatingSystemVersion));
    }

    // Set minor operating system version
    void NtHeaders::SetMinorOperatingSystemVersion(
      WORD MinorOperatingSystemVersion) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.MinorOperatingSystemVersion), 
        MinorOperatingSystemVersion);
    }

    // Get major image version
    WORD NtHeaders::GetMajorImageVersion() const
    {
      return m_Memory.Read<WORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.MajorImageVersion));
    }

    // Set major image version
    void NtHeaders::SetMajorImageVersion(WORD MajorImageVersion) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.MajorImageVersion), MajorImageVersion);
    }

    // Get minor image version
    WORD NtHeaders::GetMinorImageVersion() const
    {
      return m_Memory.Read<WORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.MinorImageVersion));
    }

    // Set minor image version
    void NtHeaders::SetMinorImageVersion(WORD MinorImageVersion) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.MinorImageVersion), MinorImageVersion);
    }

    // Get major subsystem version
    WORD NtHeaders::GetMajorSubsystemVersion() const
    {
      return m_Memory.Read<WORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.MajorSubsystemVersion));
    }

    // Set major subsystem version
    void NtHeaders::SetMajorSubsystemVersion(WORD MajorSubsystemVersion) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.MajorSubsystemVersion), MajorSubsystemVersion);
    }

    // Get minor subsystem version
    WORD NtHeaders::GetMinorSubsystemVersion() const
    {
      return m_Memory.Read<WORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.MinorSubsystemVersion));
    }

    // Set minor subsystem version
    void NtHeaders::SetMinorSubsystemVersion(WORD MinorSubsystemVersion) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.MinorSubsystemVersion), MinorSubsystemVersion);
    }

    // Get Win32 version value
    DWORD NtHeaders::GetWin32VersionValue() const
    {
      return m_Memory.Read<DWORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.Win32VersionValue));
    }

    // Set Win32 version value
    void NtHeaders::SetWin32VersionValue(DWORD Win32VersionValue) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.Win32VersionValue), Win32VersionValue);
    }

    // Get size of image
    DWORD NtHeaders::GetSizeOfImage() const
    {
      return m_Memory.Read<DWORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.SizeOfImage));
    }

    // Set size of image
    void NtHeaders::SetSizeOfImage(DWORD SizeOfImage) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.SizeOfImage), SizeOfImage);
    }

    // Get size of headers
    DWORD NtHeaders::GetSizeOfHeaders() const
    {
      return m_Memory.Read<DWORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.SizeOfHeaders));
    }

    // Set size of headers
    void NtHeaders::SetSizeOfHeaders(DWORD SizeOfHeaders) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.SizeOfHeaders), SizeOfHeaders);
    }

    // Get checksum
    DWORD NtHeaders::GetCheckSum() const
    {
      return m_Memory.Read<DWORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.CheckSum));
    }

    // Set checksum
    void NtHeaders::SetCheckSum(DWORD CheckSum) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.CheckSum), CheckSum);
    }

    // Get subsystem
    WORD NtHeaders::GetSubsystem() const
    {
      return m_Memory.Read<WORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.Subsystem));
    }

    // Set subsystem
    void NtHeaders::SetSubsystem(WORD Subsystem) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.Subsystem), Subsystem);
    }

    // Get DLL characteristics
    WORD NtHeaders::GetDllCharacteristics() const
    {
      return m_Memory.Read<WORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.DllCharacteristics));
    }

    // Set DLL characteristics
    void NtHeaders::SetDllCharacteristics(WORD DllCharacteristics) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.DllCharacteristics), DllCharacteristics);
    }

    // Get size of stack reserve
    ULONG_PTR NtHeaders::GetSizeOfStackReserve() const
    {
      return m_Memory.Read<ULONG_PTR>(m_pBase + FIELD_OFFSET(
        IMAGE_NT_HEADERS, OptionalHeader.SizeOfStackReserve));
    }

    // Set size of stack reserve
    void NtHeaders::SetSizeOfStackReserve(ULONG_PTR SizeOfStackReserve) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.SizeOfStackReserve), SizeOfStackReserve);
    }

    // Get size of stack commit
    ULONG_PTR NtHeaders::GetSizeOfStackCommit() const
    {
      return m_Memory.Read<ULONG_PTR>(m_pBase + FIELD_OFFSET(
        IMAGE_NT_HEADERS, OptionalHeader.SizeOfStackCommit));
    }

    // Set size of stack commit
    void NtHeaders::SetSizeOfStackCommit(ULONG_PTR SizeOfStackCommit) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.SizeOfStackCommit), SizeOfStackCommit);
    }

    // Get size of heap reserve
    ULONG_PTR NtHeaders::GetSizeOfHeapReserve() const
    {
      return m_Memory.Read<ULONG_PTR>(m_pBase + FIELD_OFFSET(
        IMAGE_NT_HEADERS, OptionalHeader.SizeOfHeapReserve));
    }

    // Set size of heap reserve
    void NtHeaders::SetSizeOfHeapReserve(ULONG_PTR SizeOfHeapReserve) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.SizeOfHeapReserve), SizeOfHeapReserve);
    }

    // Get size of heap commit
    ULONG_PTR NtHeaders::GetSizeOfHeapCommit() const
    {
      return m_Memory.Read<ULONG_PTR>(m_pBase + FIELD_OFFSET(
        IMAGE_NT_HEADERS, OptionalHeader.SizeOfHeapCommit));
    }

    // Set size of heap commit
    void NtHeaders::SetSizeOfHeapCommit(ULONG_PTR SizeOfHeapCommit) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.SizeOfHeapCommit), SizeOfHeapCommit);
    }

    // Get loader flags
    DWORD NtHeaders::GetLoaderFlags() const
    {
      return m_Memory.Read<DWORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.LoaderFlags));
    }

    // Set loader flags
    void NtHeaders::SetLoaderFlags(DWORD LoaderFlags) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.LoaderFlags), LoaderFlags);
    }

    // Get number of RVA and sizes
    DWORD NtHeaders::GetNumberOfRvaAndSizes() const
    {
      return m_Memory.Read<DWORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.NumberOfRvaAndSizes));
    }

    // Set number of RVA and sizes
    void NtHeaders::SetNumberOfRvaAndSizes(DWORD NumberOfRvaAndSizes) const
    {
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.NumberOfRvaAndSizes), NumberOfRvaAndSizes);
    }

    // Get data directory virtual address
    DWORD NtHeaders::GetDataDirectoryVirtualAddress(DataDir MyDataDir) const
    {
      if (static_cast<DWORD>(MyDataDir) >= GetNumberOfRvaAndSizes())
      {
        return 0;
      }
      
      return m_Memory.Read<DWORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.DataDirectory[0]) + MyDataDir * sizeof(
        IMAGE_DATA_DIRECTORY) + FIELD_OFFSET(IMAGE_DATA_DIRECTORY, 
        VirtualAddress));
    }

    // Set data directory virtual address
    void NtHeaders::SetDataDirectoryVirtualAddress(DataDir MyDataDir, 
      DWORD DataDirectoryVirtualAddress) const
    {
      if (static_cast<DWORD>(MyDataDir) >= GetNumberOfRvaAndSizes())
      {
        return;
      }
      
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.DataDirectory[0]) + MyDataDir * sizeof(
        IMAGE_DATA_DIRECTORY) + FIELD_OFFSET(IMAGE_DATA_DIRECTORY, 
        VirtualAddress), DataDirectoryVirtualAddress);
    }

    // Get data directory size
    DWORD NtHeaders::GetDataDirectorySize(DataDir MyDataDir) const
    {
      if (static_cast<DWORD>(MyDataDir) >= GetNumberOfRvaAndSizes())
      {
        return 0;
      }
      
      return m_Memory.Read<DWORD>(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.DataDirectory[0]) + MyDataDir * sizeof(
        IMAGE_DATA_DIRECTORY) + FIELD_OFFSET(IMAGE_DATA_DIRECTORY, 
        Size));
    }

    // Set data directory size
    void NtHeaders::SetDataDirectorySize(DataDir MyDataDir, 
      DWORD DataDirectorySize) const
    {
      if (static_cast<DWORD>(MyDataDir) >= GetNumberOfRvaAndSizes())
      {
        return;
      }
      
      m_Memory.Write(m_pBase + FIELD_OFFSET(IMAGE_NT_HEADERS, 
        OptionalHeader.DataDirectory[0]) + MyDataDir * sizeof(
        IMAGE_DATA_DIRECTORY) + FIELD_OFFSET(IMAGE_DATA_DIRECTORY, Size), 
        DataDirectorySize);
    }

    // Get raw NT headers
    IMAGE_NT_HEADERS NtHeaders::GetHeadersRaw() const
    {
      return m_Memory.Read<IMAGE_NT_HEADERS>(GetBase());
    }
  }
}

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

#define BOOST_TEST_MODULE NtHeadersTest
#pragma warning(push, 1)
#include <boost/test/unit_test.hpp>
#pragma warning(pop)

#include "HadesMemory/Memory.hpp"

BOOST_AUTO_TEST_CASE(BOOST_TEST_MODULE)
{
  Hades::Memory::MemoryMgr MyMemory(GetCurrentProcessId());
    
  // Todo: Also test FileType_Data
  Hades::Memory::PeFile MyPeFile(MyMemory, GetModuleHandle(NULL));
  
  Hades::Memory::NtHeaders MyNtHdrs(MyPeFile);
    
  auto HdrRaw = MyMemory.Read<IMAGE_NT_HEADERS>(MyNtHdrs.GetBase());
  
  BOOST_CHECK_EQUAL(MyNtHdrs.IsSignatureValid(), true);
  MyNtHdrs.EnsureSignatureValid();
  MyNtHdrs.SetSignature(MyNtHdrs.GetSignature());
  MyNtHdrs.SetMachine(MyNtHdrs.GetMachine());
  MyNtHdrs.SetNumberOfSections(MyNtHdrs.GetNumberOfSections());
  MyNtHdrs.SetTimeDateStamp(MyNtHdrs.GetTimeDateStamp());
  MyNtHdrs.SetPointerToSymbolTable(MyNtHdrs.GetPointerToSymbolTable());
  MyNtHdrs.SetNumberOfSymbols(MyNtHdrs.GetNumberOfSymbols());
  MyNtHdrs.SetSizeOfOptionalHeader(MyNtHdrs.GetSizeOfOptionalHeader());
  MyNtHdrs.SetCharacteristics(MyNtHdrs.GetCharacteristics());
  MyNtHdrs.SetMagic(MyNtHdrs.GetMagic());
  MyNtHdrs.SetMajorLinkerVersion(MyNtHdrs.GetMajorLinkerVersion());
  MyNtHdrs.SetMinorLinkerVersion(MyNtHdrs.GetMinorLinkerVersion());
  MyNtHdrs.SetSizeOfCode(MyNtHdrs.GetSizeOfCode());
  MyNtHdrs.SetSizeOfInitializedData(MyNtHdrs.GetSizeOfInitializedData());
  MyNtHdrs.SetSizeOfUninitializedData(MyNtHdrs.GetSizeOfUninitializedData());
  MyNtHdrs.SetAddressOfEntryPoint(MyNtHdrs.GetAddressOfEntryPoint());
  MyNtHdrs.SetBaseOfCode(MyNtHdrs.GetBaseOfCode());
#if defined(_M_IX86) 
  MyNtHdrs.SetBaseOfData(MyNtHdrs.GetBaseOfData());
#endif
  MyNtHdrs.SetImageBase(MyNtHdrs.GetImageBase());
  MyNtHdrs.SetSectionAlignment(MyNtHdrs.GetSectionAlignment());
  MyNtHdrs.SetFileAlignment(MyNtHdrs.GetFileAlignment());
  MyNtHdrs.SetMajorOperatingSystemVersion(MyNtHdrs.
    GetMajorOperatingSystemVersion());
  MyNtHdrs.SetMinorOperatingSystemVersion(MyNtHdrs.
    GetMinorOperatingSystemVersion());
  MyNtHdrs.SetMajorImageVersion(MyNtHdrs.GetMajorImageVersion());
  MyNtHdrs.SetMinorImageVersion(MyNtHdrs.GetMinorImageVersion());
  MyNtHdrs.SetMajorSubsystemVersion(MyNtHdrs.GetMajorSubsystemVersion());
  MyNtHdrs.SetMinorSubsystemVersion(MyNtHdrs.GetMinorSubsystemVersion());
  MyNtHdrs.SetWin32VersionValue(MyNtHdrs.GetWin32VersionValue());
  MyNtHdrs.SetSizeOfImage(MyNtHdrs.GetSizeOfImage());
  MyNtHdrs.SetSizeOfHeaders(MyNtHdrs.GetSizeOfHeaders());
  MyNtHdrs.SetCheckSum(MyNtHdrs.GetCheckSum());
  MyNtHdrs.SetSubsystem(MyNtHdrs.GetSubsystem());
  MyNtHdrs.SetDllCharacteristics(MyNtHdrs.GetDllCharacteristics());
  MyNtHdrs.SetSizeOfStackReserve(MyNtHdrs.GetSizeOfStackReserve());
  MyNtHdrs.SetSizeOfStackCommit(MyNtHdrs.GetSizeOfStackCommit());
  MyNtHdrs.SetSizeOfHeapReserve(MyNtHdrs.GetSizeOfHeapReserve());
  MyNtHdrs.SetSizeOfHeapCommit(MyNtHdrs.GetSizeOfHeapCommit());
  MyNtHdrs.SetLoaderFlags(MyNtHdrs.GetLoaderFlags());
  MyNtHdrs.SetNumberOfRvaAndSizes(MyNtHdrs.GetNumberOfRvaAndSizes());
  // Todo: Investigate whether this should be checking NumberOfRvaAndSizes 
  // instead of IMAGE_NUMBEROF_DIRECTORY_ENTRIES. Especially when working 
  // with an on-disk representation of a PE file.
  for (std::size_t i = 0; i < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; ++i)
  {
    auto Dir = static_cast<Hades::Memory::NtHeaders::DataDir>(i);
    MyNtHdrs.SetDataDirectoryVirtualAddress(Dir, MyNtHdrs.
      GetDataDirectoryVirtualAddress(Dir));
    MyNtHdrs.SetDataDirectorySize(Dir, MyNtHdrs.GetDataDirectorySize(Dir));
  }
  
  MyNtHdrs.GetHeadersRaw();
    
  auto HdrRawNew = MyMemory.Read<IMAGE_NT_HEADERS>(MyNtHdrs.GetBase());
  
  BOOST_CHECK_EQUAL(std::memcmp(&HdrRaw, &HdrRawNew, sizeof(
    IMAGE_NT_HEADERS)), 0);
}

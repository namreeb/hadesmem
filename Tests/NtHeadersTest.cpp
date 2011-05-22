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
#include "HadesMemory/Module.hpp"
#include "HadesMemory/MemoryMgr.hpp"
#include "HadesMemory/PeLib/PeFile.hpp"
#include "HadesMemory/PeLib/DosHeader.hpp"
#include "HadesMemory/PeLib/NtHeaders.hpp"

// Boost
#define BOOST_TEST_MODULE NtHeadersTest
#include <boost/test/unit_test.hpp>

// NT headers component tests
BOOST_AUTO_TEST_CASE(BOOST_TEST_MODULE)
{
  // Create memory manager for self
  Hades::Memory::MemoryMgr MyMemory(GetCurrentProcessId());
    
  // Enumerate module list and run NT headers tests on all modules
  Hades::Memory::ModuleList Modules(MyMemory);
  std::for_each(Modules.begin(), Modules.end(), 
    [&] (Hades::Memory::Module const& Mod) 
    {
      // Open module as a memory-based PeFile
      // Todo: Also test FileType_Data
      Hades::Memory::PeFile MyPeFile(MyMemory, Mod.GetBase());
      Hades::Memory::DosHeader const MyDosHeader(MyPeFile);
      Hades::Memory::NtHeaders const MyNtHeaders(MyPeFile);
    
      // Get raw NT headers
      auto HdrRaw = MyMemory.Read<IMAGE_NT_HEADERS>(MyNtHeaders.GetBase());
      
      // Ensure all member functions are called without exception, and 
      // overwrite the value of each field with the existing value
      BOOST_CHECK_EQUAL(MyNtHeaders.IsSignatureValid(), true);
      MyNtHeaders.EnsureSignatureValid();
      MyNtHeaders.SetSignature(MyNtHeaders.GetSignature());
      MyNtHeaders.SetMachine(MyNtHeaders.GetMachine());
      MyNtHeaders.SetNumberOfSections(MyNtHeaders.GetNumberOfSections());
      MyNtHeaders.SetTimeDateStamp(MyNtHeaders.GetTimeDateStamp());
      MyNtHeaders.SetPointerToSymbolTable(MyNtHeaders.
        GetPointerToSymbolTable());
      MyNtHeaders.SetNumberOfSymbols(MyNtHeaders.GetNumberOfSymbols());
      MyNtHeaders.SetSizeOfOptionalHeader(MyNtHeaders.
        GetSizeOfOptionalHeader());
      MyNtHeaders.SetCharacteristics(MyNtHeaders.GetCharacteristics());
      MyNtHeaders.SetMagic(MyNtHeaders.GetMagic());
      MyNtHeaders.SetMajorLinkerVersion(MyNtHeaders.GetMajorLinkerVersion());
      MyNtHeaders.SetMinorLinkerVersion(MyNtHeaders.GetMinorLinkerVersion());
      MyNtHeaders.SetSizeOfCode(MyNtHeaders.GetSizeOfCode());
      MyNtHeaders.SetSizeOfInitializedData(MyNtHeaders.
        GetSizeOfInitializedData());
      MyNtHeaders.SetSizeOfUninitializedData(MyNtHeaders.
        GetSizeOfUninitializedData());
      MyNtHeaders.SetAddressOfEntryPoint(MyNtHeaders.GetAddressOfEntryPoint());
      MyNtHeaders.SetBaseOfCode(MyNtHeaders.GetBaseOfCode());
    #if defined(_M_IX86) 
      MyNtHeaders.SetBaseOfData(MyNtHeaders.GetBaseOfData());
    #endif
      MyNtHeaders.SetImageBase(MyNtHeaders.GetImageBase());
      MyNtHeaders.SetSectionAlignment(MyNtHeaders.GetSectionAlignment());
      MyNtHeaders.SetFileAlignment(MyNtHeaders.GetFileAlignment());
      MyNtHeaders.SetMajorOperatingSystemVersion(MyNtHeaders.
        GetMajorOperatingSystemVersion());
      MyNtHeaders.SetMinorOperatingSystemVersion(MyNtHeaders.
        GetMinorOperatingSystemVersion());
      MyNtHeaders.SetMajorImageVersion(MyNtHeaders.GetMajorImageVersion());
      MyNtHeaders.SetMinorImageVersion(MyNtHeaders.GetMinorImageVersion());
      MyNtHeaders.SetMajorSubsystemVersion(MyNtHeaders.
        GetMajorSubsystemVersion());
      MyNtHeaders.SetMinorSubsystemVersion(MyNtHeaders.
        GetMinorSubsystemVersion());
      MyNtHeaders.SetWin32VersionValue(MyNtHeaders.GetWin32VersionValue());
      MyNtHeaders.SetSizeOfImage(MyNtHeaders.GetSizeOfImage());
      MyNtHeaders.SetSizeOfHeaders(MyNtHeaders.GetSizeOfHeaders());
      MyNtHeaders.SetCheckSum(MyNtHeaders.GetCheckSum());
      MyNtHeaders.SetSubsystem(MyNtHeaders.GetSubsystem());
      MyNtHeaders.SetDllCharacteristics(MyNtHeaders.GetDllCharacteristics());
      MyNtHeaders.SetSizeOfStackReserve(MyNtHeaders.GetSizeOfStackReserve());
      MyNtHeaders.SetSizeOfStackCommit(MyNtHeaders.GetSizeOfStackCommit());
      MyNtHeaders.SetSizeOfHeapReserve(MyNtHeaders.GetSizeOfHeapReserve());
      MyNtHeaders.SetSizeOfHeapCommit(MyNtHeaders.GetSizeOfHeapCommit());
      MyNtHeaders.SetLoaderFlags(MyNtHeaders.GetLoaderFlags());
      MyNtHeaders.SetNumberOfRvaAndSizes(MyNtHeaders.GetNumberOfRvaAndSizes());
      // Todo: Investigate whether this should be checking NumberOfRvaAndSizes 
      // instead of IMAGE_NUMBEROF_DIRECTORY_ENTRIES. Especially when working 
      // with an on-disk representation of a PE file.
      for (std::size_t i = 0; i < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; ++i)
      {
        auto Dir = static_cast<Hades::Memory::NtHeaders::DataDir>(i);
        MyNtHeaders.SetDataDirectoryVirtualAddress(Dir, MyNtHeaders.
          GetDataDirectoryVirtualAddress(Dir));
        MyNtHeaders.SetDataDirectorySize(Dir, MyNtHeaders.GetDataDirectorySize(
          Dir));
      }
      
      // Get raw TLS dir data again (using the member function this time)
      auto HdrRawNew = MyNtHeaders.GetHeadersRaw();
      
      // Ensure NtHeaders getters/setters 'match' by checking that the data is 
      // unchanged
      BOOST_CHECK_EQUAL(std::memcmp(&HdrRaw, &HdrRawNew, sizeof(
        HdrRaw)), 0);
    });
}

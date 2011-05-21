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
    
      // Create NT headers manager
      Hades::Memory::NtHeaders MyNtHdrs(MyPeFile);
        
      // Get raw NT headers
      auto HdrRaw = MyMemory.Read<IMAGE_NT_HEADERS>(MyNtHdrs.GetBase());
      
      // Ensure all member functions are called without exception, and 
      // overwrite the value of each field with the existing value
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
      
      // Get raw TLS dir data again (using the member function this time)
      auto HdrRawNew = MyNtHdrs.GetHeadersRaw();
      
      // Ensure NtHeaders getters/setters 'match' by checking that the data is 
      // unchanged
      BOOST_CHECK_EQUAL(std::memcmp(&HdrRaw, &HdrRawNew, sizeof(
        HdrRaw)), 0);
    });
}

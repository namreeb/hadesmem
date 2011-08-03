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
#include <HadesMemory/Module.hpp>
#include <HadesMemory/MemoryMgr.hpp>
#include <HadesMemory/PeLib/PeFile.hpp>
#include <HadesMemory/PeLib/DosHeader.hpp>
#include <HadesMemory/PeLib/NtHeaders.hpp>

// Boost
#define BOOST_TEST_MODULE NtHeadersTest
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(ConstructorsTest)
{
  // Create memory manager for self
  HadesMem::MemoryMgr MyMemory(GetCurrentProcessId());
    
  // Create PeFile
  HadesMem::PeFile MyPeFile(MyMemory, GetModuleHandle(NULL));
    
  // Create NT headers
  HadesMem::NtHeaders MyNtHeaders(MyPeFile);
  
  // Test copying, assignement, and moving
  HadesMem::NtHeaders OtherNtHeaders(MyNtHeaders);
  BOOST_CHECK(MyNtHeaders == OtherNtHeaders);
  MyNtHeaders = OtherNtHeaders;
  BOOST_CHECK(MyNtHeaders == OtherNtHeaders);
  HadesMem::NtHeaders MovedNtHeaders(std::move(OtherNtHeaders));
  BOOST_CHECK(MovedNtHeaders == MyNtHeaders);
  HadesMem::NtHeaders NewTestNtHeaders(MyNtHeaders);
  MyNtHeaders = std::move(NewTestNtHeaders);
  BOOST_CHECK(MyNtHeaders == MovedNtHeaders);
}

BOOST_AUTO_TEST_CASE(DataTest)
{
  // Create memory manager for self
  HadesMem::MemoryMgr const MyMemory(GetCurrentProcessId());
    
  // Enumerate module list and run NT headers tests on all modules
  HadesMem::ModuleList Modules(MyMemory);
  std::for_each(Modules.begin(), Modules.end(), 
    [&] (HadesMem::Module const& Mod) 
    {
      // Open module as a memory-based PeFile
      // Todo: Also test FileType_Data
      HadesMem::PeFile const MyPeFile(MyMemory, Mod.GetHandle());
      HadesMem::DosHeader const MyDosHeader(MyPeFile);
      HadesMem::NtHeaders const MyNtHeaders(MyPeFile);
    
      // Get raw NT headers
      auto const HdrRaw = MyMemory.Read<IMAGE_NT_HEADERS>(
        MyNtHeaders.GetBase());
      
      // Ensure all member functions are called without exception, and 
      // overwrite the value of each field with the existing value
      BOOST_CHECK_EQUAL(MyNtHeaders.IsValid(), true);
      MyNtHeaders.EnsureValid();
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
      for (std::size_t i = 0; i < MyNtHeaders.GetNumberOfRvaAndSizes(); ++i)
      {
        auto Dir = static_cast<HadesMem::NtHeaders::DataDir>(i);
        MyNtHeaders.SetDataDirectoryVirtualAddress(Dir, MyNtHeaders.
          GetDataDirectoryVirtualAddress(Dir));
        MyNtHeaders.SetDataDirectorySize(Dir, MyNtHeaders.GetDataDirectorySize(
          Dir));
      }
      
      // Get raw TLS dir data again
      auto const HdrRawNew = MyMemory.Read<IMAGE_NT_HEADERS>(
        MyNtHeaders.GetBase());
      
      // Ensure NtHeaders getters/setters 'match' by checking that the data is 
      // unchanged
      BOOST_CHECK_EQUAL(std::memcmp(&HdrRaw, &HdrRawNew, sizeof(
        HdrRaw)), 0);
    });
}

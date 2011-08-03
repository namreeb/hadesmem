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
#include <HadesMemory/PeLib/DosHeader.hpp>
#include <HadesMemory/Module.hpp>
#include <HadesMemory/MemoryMgr.hpp>
#include <HadesMemory/PeLib/PeFile.hpp>

// Boost
#define BOOST_TEST_MODULE DosHeaderTest
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(ConstructorsTest)
{
  // Create memory manager for self
  HadesMem::MemoryMgr MyMemory(GetCurrentProcessId());
    
  // Create PeFile
  HadesMem::PeFile MyPeFile(MyMemory, GetModuleHandle(NULL));
    
  // Create DOS header
  HadesMem::DosHeader MyDosHeader(MyPeFile);
  
  // Test copying, assignement, and moving
  HadesMem::DosHeader OtherDosHeader(MyDosHeader);
  BOOST_CHECK(MyDosHeader == OtherDosHeader);
  MyDosHeader = OtherDosHeader;
  BOOST_CHECK(MyDosHeader == OtherDosHeader);
  HadesMem::DosHeader MovedDosHeader(std::move(OtherDosHeader));
  BOOST_CHECK(MovedDosHeader == MyDosHeader);
  HadesMem::DosHeader NewTestDosHeader(MyDosHeader);
  MyDosHeader = std::move(NewTestDosHeader);
  BOOST_CHECK(MyDosHeader == MovedDosHeader);
}

BOOST_AUTO_TEST_CASE(DataTests)
{
  // Create memory manager for self
  HadesMem::MemoryMgr const MyMemory(GetCurrentProcessId());
  
  // Enumerate module list and run DOS header tests on all modules
  HadesMem::ModuleList Modules(MyMemory);
  std::for_each(Modules.cbegin(), Modules.cend(), 
    [&] (HadesMem::Module const& Mod) 
    {
      // Open module as a memory-based PeFile
      // Todo: Also test FileType_Data
      HadesMem::PeFile const MyPeFile(MyMemory, Mod.GetHandle());
      
      // Create DOS header manager
      HadesMem::DosHeader MyDosHdr(MyPeFile);
        
      // Get raw DOS header
      auto const HdrRaw = MyMemory.Read<IMAGE_DOS_HEADER>(MyPeFile.GetBase());
      
      // Ensure all member functions are called without exception, and 
      // overwrite the value of each field with the existing value
      BOOST_CHECK_EQUAL(MyDosHdr.IsValid(), true);
      MyDosHdr.EnsureValid();
      MyDosHdr.SetMagic(MyDosHdr.GetMagic());
      MyDosHdr.SetBytesOnLastPage(MyDosHdr.GetBytesOnLastPage());
      MyDosHdr.SetPagesInFile(MyDosHdr.GetPagesInFile());
      MyDosHdr.SetRelocations(MyDosHdr.GetRelocations());
      MyDosHdr.SetSizeOfHeaderInParagraphs(MyDosHdr.
        GetSizeOfHeaderInParagraphs());
      MyDosHdr.SetMinExtraParagraphs(MyDosHdr.GetMinExtraParagraphs());
      MyDosHdr.SetMaxExtraParagraphs(MyDosHdr.GetMaxExtraParagraphs());
      MyDosHdr.SetInitialSS(MyDosHdr.GetInitialSS());
      MyDosHdr.SetInitialSP(MyDosHdr.GetInitialSP());
      MyDosHdr.SetChecksum(MyDosHdr.GetChecksum());
      MyDosHdr.SetInitialIP(MyDosHdr.GetInitialIP());
      MyDosHdr.SetInitialCS(MyDosHdr.GetInitialCS());
      MyDosHdr.SetRelocTableFileAddr(MyDosHdr.GetRelocTableFileAddr());
      MyDosHdr.SetOverlayNum(MyDosHdr.GetOverlayNum());
      MyDosHdr.SetReservedWords1(MyDosHdr.GetReservedWords1());
      MyDosHdr.SetOEMID(MyDosHdr.GetOEMID());
      MyDosHdr.SetOEMInfo(MyDosHdr.GetOEMInfo());
      MyDosHdr.SetReservedWords2(MyDosHdr.GetReservedWords2());
      MyDosHdr.SetNewHeaderOffset(MyDosHdr.GetNewHeaderOffset());
      
      // Get raw DOS header again
      auto const HdrRawNew = MyMemory.Read<IMAGE_DOS_HEADER>(
        MyPeFile.GetBase());
      
      // Ensure DosHeader getters/setters 'match' by checking that the data is 
      // unchanged
      BOOST_CHECK_EQUAL(std::memcmp(&HdrRaw, &HdrRawNew, sizeof(
        HdrRaw)), 0);
    });
}

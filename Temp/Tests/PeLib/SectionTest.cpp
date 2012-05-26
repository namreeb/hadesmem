// Copyright Joshua Boyce 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

// Hades
#include <HadesMemory/Module.hpp>
#include <HadesMemory/MemoryMgr.hpp>
#include <HadesMemory/PeLib/PeFile.hpp>
#include <HadesMemory/PeLib/Section.hpp>
#include <HadesMemory/PeLib/DosHeader.hpp>
#include <HadesMemory/PeLib/NtHeaders.hpp>

// C++ Standard Library
#include <algorithm>

// Boost
#define BOOST_TEST_MODULE SectionTest
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(ConstructorsTest)
{
  // Create memory manager for self
  HadesMem::MemoryMgr MyMemory(GetCurrentProcessId());
    
  // Create PeFile
  HadesMem::PeFile MyPeFile(MyMemory, GetModuleHandle(NULL));
    
  // Create section
  HadesMem::Section MySection(MyPeFile, 0);
  
  // Test copying, assignement, and moving
  HadesMem::Section OtherSection(MySection);
  BOOST_CHECK(MySection == OtherSection);
  MySection = OtherSection;
  BOOST_CHECK(MySection == OtherSection);
  HadesMem::Section MovedSection(std::move(OtherSection));
  BOOST_CHECK(MovedSection == MySection);
  HadesMem::Section NewTestSection(MySection);
  MySection = std::move(NewTestSection);
  BOOST_CHECK(MySection == MovedSection);
}

BOOST_AUTO_TEST_CASE(DataTest)
{
  // Create memory manager for self
  HadesMem::MemoryMgr const MyMemory(GetCurrentProcessId());
    
  // Enumerate module list and run section tests on all modules
  HadesMem::ModuleList Modules(MyMemory);
  std::for_each(Modules.begin(), Modules.end(), 
    [&] (HadesMem::Module const& Mod) 
    {
      // Open module as a memory-based PeFile
      // Todo: Also test FileType_Data
      HadesMem::PeFile const MyPeFile(MyMemory, Mod.GetHandle());
      HadesMem::DosHeader const MyDosHeader(MyPeFile);
      HadesMem::NtHeaders const MyNtHeaders(MyPeFile);
      
      // Enumerate sections for module
      WORD Number = 0;
      HadesMem::SectionList Sections(MyPeFile);
      if (Mod.GetHandle() == GetModuleHandle(NULL))
      {
        BOOST_CHECK(Sections.begin() != Sections.end());
        
        auto Iter = std::find_if(Sections.cbegin(), Sections.cend(), 
          [] (HadesMem::Section const& S)
          {
            return S.GetName() == ".text";
          });
        BOOST_CHECK(Iter != Sections.cend());
      }
      std::for_each(Sections.begin(), Sections.end(), 
        [&] (HadesMem::Section const& S)
        {
          // Check Section::GetNumber
          BOOST_CHECK_EQUAL(S.GetNumber(), Number);
          ++Number;
          
          // Test Section::Section
          HadesMem::Section Test(MyPeFile, S.GetNumber());
          
          // Get raw section header data
          auto const HdrRaw = MyMemory.Read<IMAGE_SECTION_HEADER>(
            Test.GetBase());
          
          // Ensure all member functions are called without exception, and 
          // overwrite the value of each field with the existing value
          Test.SetName(Test.GetName());
          Test.SetVirtualAddress(Test.GetVirtualAddress());
          Test.SetVirtualSize(Test.GetVirtualSize());
          Test.SetSizeOfRawData(Test.GetSizeOfRawData());
          Test.SetPointerToRawData(Test.GetPointerToRawData());
          Test.SetPointerToRelocations(Test.GetPointerToRelocations());
          Test.SetPointerToLinenumbers(Test.GetPointerToLinenumbers());
          Test.SetNumberOfRelocations(Test.GetNumberOfRelocations());
          Test.SetNumberOfLinenumbers(Test.GetNumberOfLinenumbers());
          Test.SetCharacteristics(Test.GetCharacteristics());
            
          // Get raw section header data again
          auto const HdrRawNew = MyMemory.Read<IMAGE_SECTION_HEADER>(
            Test.GetBase());
          
          // Ensure TlsDir getters/setters 'match' by checking that the data is 
          // unchanged
          BOOST_CHECK_EQUAL(std::memcmp(&HdrRaw, &HdrRawNew, sizeof(
            HdrRaw)), 0);
        });
    });
}

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

#define BOOST_TEST_MODULE SectionTest
#include <boost/test/unit_test.hpp>

#include <algorithm>

#include "HadesMemory/Section.hpp"
#include "HadesMemory/Module.hpp"
#include "HadesMemory/PeFile.hpp"
#include "HadesMemory/MemoryMgr.hpp"

BOOST_AUTO_TEST_CASE(BOOST_TEST_MODULE)
{
  Hades::Memory::MemoryMgr MyMemory(GetCurrentProcessId());
    
  Hades::Memory::ModuleList Modules(MyMemory);
  std::for_each(Modules.begin(), Modules.end(), 
    [&] (Hades::Memory::Module const& Mod) 
    {
      // Todo: Also test FileType_Data
      Hades::Memory::PeFile MyPeFile(MyMemory, Mod.GetBase());
      
      Hades::Memory::SectionList Sections(MyPeFile);
      std::for_each(Sections.begin(), Sections.end(), 
        [&] (Hades::Memory::Section& S)
        {
          Hades::Memory::Section Test(MyPeFile, S.GetNumber());
          
          auto HdrRaw = MyMemory.Read<IMAGE_SECTION_HEADER>(Test.GetBase());
          
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
            
          auto HdrRawNew = MyMemory.Read<IMAGE_SECTION_HEADER>(Test.GetBase());
          
          BOOST_CHECK_EQUAL(std::memcmp(&HdrRaw, &HdrRawNew, sizeof(
            IMAGE_SECTION_HEADER)), 0);
        });
    });
}

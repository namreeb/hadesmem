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

#define BOOST_TEST_MODULE ImportDirTest
#pragma warning(push, 1)
#include <boost/test/unit_test.hpp>
#pragma warning(pop)

#include "HadesMemory/Memory.hpp"

BOOST_AUTO_TEST_CASE(BOOST_TEST_MODULE)
{
  Hades::Memory::MemoryMgr MyMemory(GetCurrentProcessId());
    
  for (Hades::Memory::ModuleIter ModIter(MyMemory); *ModIter; ++ModIter)
  {
    Hades::Memory::Module const Mod = **ModIter;
      
    // Todo: Also test FileType_Data
    Hades::Memory::PeFile MyPeFile(MyMemory, Mod.GetBase());
      
    boost::optional<Hades::Memory::ImportDir> TestEnum(*Hades::Memory::
      ImportDirIter(MyPeFile));
    BOOST_CHECK(TestEnum);
      
    for (Hades::Memory::ImportDirIter i(MyPeFile); *i; ++i)
    {
      Hades::Memory::ImportDir Current = **i;
        
      if (!Current.IsValid())
      {
        continue;
      }
      
      Hades::Memory::ImportDir Test(MyPeFile, 
        reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(Current.GetBase()));
      
      auto ImpDirRaw = MyMemory.Read<IMAGE_IMPORT_DESCRIPTOR>(Test.GetBase());
      
      BOOST_CHECK_EQUAL(Test.IsValid(), true);
      Test.EnsureValid();
      Test.SetCharacteristics(Test.GetCharacteristics());
      Test.SetTimeDateStamp(Test.GetTimeDateStamp());
      Test.SetForwarderChain(Test.GetForwarderChain());
      Test.SetNameRaw(Test.GetNameRaw());
      Test.SetFirstThunk(Test.GetFirstThunk());
      BOOST_CHECK(!Test.GetName().empty());
      
      auto ImpDirRawNew = MyMemory.Read<IMAGE_IMPORT_DESCRIPTOR>(
        Test.GetBase());
        
      BOOST_CHECK_EQUAL(std::memcmp(&ImpDirRaw, &ImpDirRawNew, sizeof(
        IMAGE_IMPORT_DESCRIPTOR)), 0);
        
      for (Hades::Memory::ImportThunkIter j(MyPeFile, Test.GetCharacteristics()); 
        *j; ++j)
      {
        Hades::Memory::ImportThunk CurrentNew = **j;
        Hades::Memory::ImportThunk TestNew(MyPeFile, CurrentNew.GetBase());
        
        auto ImpThunkRaw = MyMemory.Read<IMAGE_THUNK_DATA>(TestNew.GetBase());
        
        BOOST_CHECK_EQUAL(TestNew.IsValid(), true);
        TestNew.EnsureValid();
        TestNew.SetAddressOfData(TestNew.GetAddressOfData());
        TestNew.SetOrdinalRaw(TestNew.GetOrdinalRaw());
        TestNew.SetFunction(TestNew.GetFunction());
        TestNew.SetHint(TestNew.GetHint());
        TestNew.GetBase();
        if (TestNew.ByOrdinal())
        {
          TestNew.GetOrdinal();
        }
        else
        {
          BOOST_CHECK(!TestNew.GetName().empty());
        }
        
        auto ImpThunkRawNew = MyMemory.Read<IMAGE_THUNK_DATA>(
          TestNew.GetBase());
          
        BOOST_CHECK_EQUAL(std::memcmp(&ImpThunkRaw, &ImpThunkRawNew, sizeof(
          IMAGE_THUNK_DATA)), 0);
      }
    }
  }
}

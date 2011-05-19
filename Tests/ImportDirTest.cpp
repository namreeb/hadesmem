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
#include <boost/test/unit_test.hpp>

#include "HadesMemory/Module.hpp"
#include "HadesMemory/MemoryMgr.hpp"
#include "HadesMemory/PeLib/PeFile.hpp"
#include "HadesMemory/PeLib/ImportDir.hpp"

BOOST_AUTO_TEST_CASE(BOOST_TEST_MODULE)
{
  Hades::Memory::MemoryMgr MyMemory(GetCurrentProcessId());
    
  Hades::Memory::ModuleList Modules(MyMemory);
  std::for_each(Modules.begin(), Modules.end(), 
    [&] (Hades::Memory::Module const& Mod) 
    {
      // Todo: Also test FileType_Data
      Hades::Memory::PeFile MyPeFile(MyMemory, Mod.GetBase());
      
      Hades::Memory::ImportDirList ImportDirs(MyPeFile);
      std::for_each(ImportDirs.begin(), ImportDirs.end(), 
        [&] (Hades::Memory::ImportDir& D)
        {
          Hades::Memory::ImportDir Test(MyPeFile, 
            reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(D.GetBase()));
          
          auto ImpDirRaw = MyMemory.Read<IMAGE_IMPORT_DESCRIPTOR>(Test.
            GetBase());
          
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
            
          Hades::Memory::ImportThunkList ImportThunks(MyPeFile, D.GetCharacteristics());
          std::for_each(ImportThunks.begin(), ImportThunks.end(), 
            [&] (Hades::Memory::ImportThunk& T)
            {
              Hades::Memory::ImportThunk TestNew(MyPeFile, T.GetBase());
              
              auto ImpThunkRaw = MyMemory.Read<IMAGE_THUNK_DATA>(TestNew.GetBase());
              
              BOOST_CHECK_EQUAL(TestNew.IsValid(), true);
              TestNew.EnsureValid();
              TestNew.SetAddressOfData(TestNew.GetAddressOfData());
              TestNew.SetOrdinalRaw(TestNew.GetOrdinalRaw());
              TestNew.SetFunction(TestNew.GetFunction());
              TestNew.GetBase();
              if (TestNew.ByOrdinal())
              {
                TestNew.GetOrdinal();
              }
              else
              {
                TestNew.GetHint();
                BOOST_CHECK(!TestNew.GetName().empty());
              }
              
              auto ImpThunkRawNew = MyMemory.Read<IMAGE_THUNK_DATA>(
                TestNew.GetBase());
                
              BOOST_CHECK_EQUAL(std::memcmp(&ImpThunkRaw, &ImpThunkRawNew, sizeof(
                IMAGE_THUNK_DATA)), 0);
            });
        });
    });
}

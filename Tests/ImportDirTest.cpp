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
#include <HadesMemory/PeLib/ImportDir.hpp>
#include <HadesMemory/PeLib/DosHeader.hpp>
#include <HadesMemory/PeLib/NtHeaders.hpp>

// Boost
#define BOOST_TEST_MODULE ImportDirTest
#include <boost/test/unit_test.hpp>

// ImportDir component tests
BOOST_AUTO_TEST_CASE(BOOST_TEST_MODULE)
{
  // Create memory manager for self
  Hades::Memory::MemoryMgr MyMemory(GetCurrentProcessId());
  
  // Enumerate module list and run section tests on all modules
  Hades::Memory::ModuleList Modules(MyMemory);
  std::for_each(Modules.begin(), Modules.end(), 
    [&] (Hades::Memory::Module const& Mod) 
    {
      // Open module as a memory-based PeFile
      // Todo: Also test FileType_Data
      Hades::Memory::PeFile MyPeFile(MyMemory, Mod.GetBase());
      Hades::Memory::DosHeader const MyDosHeader(MyPeFile);
      Hades::Memory::NtHeaders const MyNtHeaders(MyPeFile);
      
      // Enumerate import dirs for module
      // Todo: Ensure via test that at least one module with an import dir is 
      // processed (i.e. this one)
      // Todo: Test import dir enumeration APIs before relying on them
      Hades::Memory::ImportDirList ImportDirs(MyPeFile);
      std::for_each(ImportDirs.begin(), ImportDirs.end(), 
        [&] (Hades::Memory::ImportDir& D)
        {
          // Test ImportDir constructor
          Hades::Memory::ImportDir Test(MyPeFile, 
            reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(D.GetBase()));
          
          // Get raw import dir data
          auto ImpDirRaw = MyMemory.Read<IMAGE_IMPORT_DESCRIPTOR>(Test.
            GetBase());
          
          // Ensure all member functions are called without exception, and 
          // overwrite the value of each field with the existing value
          BOOST_CHECK_EQUAL(Test.IsValid(), true);
          Test.EnsureValid();
          Test.SetCharacteristics(Test.GetCharacteristics());
          Test.SetTimeDateStamp(Test.GetTimeDateStamp());
          Test.SetForwarderChain(Test.GetForwarderChain());
          Test.SetNameRaw(Test.GetNameRaw());
          Test.SetFirstThunk(Test.GetFirstThunk());
          BOOST_CHECK(!Test.GetName().empty());
          
          // Get raw import dir data again
          auto ImpDirRawNew = MyMemory.Read<IMAGE_IMPORT_DESCRIPTOR>(
            Test.GetBase());
          
          // Ensure ImportDir getters/setters 'match' by checking that the 
          // data is unchanged
          BOOST_CHECK_EQUAL(std::memcmp(&ImpDirRaw, &ImpDirRawNew, sizeof(
            IMAGE_IMPORT_DESCRIPTOR)), 0);
          
          // Enumerate import thunks for import dir
          // Todo: Ensure via test that at least one import dir with import 
          // thunks is processed (i.e. this one)
          // Todo: Test import thunk enumeration APIs before relying on them
          Hades::Memory::ImportThunkList ImportThunks(MyPeFile, 
            D.GetCharacteristics());
          std::for_each(ImportThunks.begin(), ImportThunks.end(), 
            [&] (Hades::Memory::ImportThunk& T)
            {
              // Test ImportThunk constructor
              Hades::Memory::ImportThunk TestNew(MyPeFile, T.GetBase());
              
              // Get raw import thunk data
              auto ImpThunkRaw = MyMemory.Read<IMAGE_THUNK_DATA>(
                TestNew.GetBase());
              
              // Ensure all member functions are called without exception, and 
              // overwrite the value of each field with the existing value
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
              
              // Get raw import thunk data again
              auto ImpThunkRawNew = MyMemory.Read<IMAGE_THUNK_DATA>(
                TestNew.GetBase());
                
              // Ensure ImportThunk getters/setters 'match' by checking that 
              // the data is unchanged
              BOOST_CHECK_EQUAL(std::memcmp(&ImpThunkRaw, &ImpThunkRawNew, 
                sizeof(ImpThunkRaw)), 0);
            });
        });
    });
}

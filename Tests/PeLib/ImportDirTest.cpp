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

BOOST_AUTO_TEST_CASE(ConstructorsTest)
{
  // Create memory manager for self
  HadesMem::MemoryMgr MyMemory(GetCurrentProcessId());
    
  // Create PeFile
  HadesMem::PeFile MyPeFile(MyMemory, GetModuleHandle(NULL));
    
  // Create import dir
  HadesMem::ImportDir MyImportDir(MyPeFile);
  
  // Test copying, assignement, and moving
  HadesMem::ImportDir OtherImportDir(MyImportDir);
  BOOST_CHECK(MyImportDir == OtherImportDir);
  MyImportDir = OtherImportDir;
  BOOST_CHECK(MyImportDir == OtherImportDir);
  HadesMem::ImportDir MovedImportDir(std::move(OtherImportDir));
  BOOST_CHECK(MovedImportDir == MyImportDir);
  HadesMem::ImportDir NewTestImportDir(MyImportDir);
  MyImportDir = std::move(NewTestImportDir);
  BOOST_CHECK(MyImportDir == MovedImportDir);
  
  // TODO: Test ImportThunk constructors
}

// ImportDir component tests
BOOST_AUTO_TEST_CASE(DataTest)
{
  // Create memory manager for self
  HadesMem::MemoryMgr const MyMemory(GetCurrentProcessId());
  
  // Enumerate module list and run section tests on all modules
  HadesMem::ModuleList Modules(MyMemory);
  std::for_each(Modules.cbegin(), Modules.cend(), 
    [&] (HadesMem::Module const& Mod) 
    {
      // Open module as a memory-based PeFile
      // Todo: Also test FileType_Data
      HadesMem::PeFile const MyPeFile(MyMemory, Mod.GetHandle());
      HadesMem::DosHeader const MyDosHeader(MyPeFile);
      HadesMem::NtHeaders const MyNtHeaders(MyPeFile);
      
      // Enumerate import dirs for module
      HadesMem::ImportDirList ImportDirs(MyPeFile);
      if (Mod.GetHandle() == GetModuleHandle(NULL))
      {
        BOOST_CHECK(ImportDirs.cbegin() != ImportDirs.cend());
        
        auto Iter = std::find_if(ImportDirs.cbegin(), ImportDirs.cend(), 
          [] (HadesMem::ImportDir const& I)
          {
            return I.GetName() == "kernel32" || 
              I.GetName() == "kernel32.dll" || 
              I.GetName() == "KERNEL32.dll" || 
              I.GetName() == "KERNEL32.DLL";
          });
        BOOST_CHECK(Iter != ImportDirs.cend());
        
        HadesMem::ImportThunkList ImportThunks(MyPeFile, 
          Iter->GetCharacteristics());
        auto Iter2 = std::find_if(ImportThunks.cbegin(), ImportThunks.cend(), 
          [] (HadesMem::ImportThunk const& I)
          {
            return I.ByOrdinal() ? false : I.GetName() == 
              "GetCurrentProcessId";
          });
        BOOST_CHECK(Iter2 != ImportThunks.cend());
      }
      std::for_each(ImportDirs.begin(), ImportDirs.end(), 
        [&] (HadesMem::ImportDir const& D)
        {
          // Test ImportDir constructor
          HadesMem::ImportDir Test(MyPeFile, 
            reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(D.GetBase()));
          
          // Get raw import dir data
          auto const ImpDirRaw = MyMemory.Read<IMAGE_IMPORT_DESCRIPTOR>(Test.
            GetBase());
          
          // Ensure all member functions are called without exception, and 
          // overwrite the value of each field with the existing value
          BOOST_CHECK_EQUAL(Test.IsValid(), true);
          Test.EnsureValid();
          Test.SetCharacteristics(Test.GetCharacteristics());
          Test.SetTimeDateStamp(Test.GetTimeDateStamp());
          Test.SetForwarderChain(Test.GetForwarderChain());
          Test.SetNameRaw(Test.GetNameRaw());
          Test.SetName(Test.GetName());
          Test.SetFirstThunk(Test.GetFirstThunk());
          BOOST_CHECK(!Test.GetName().empty());
          
          // Get raw import dir data again
          auto const ImpDirRawNew = MyMemory.Read<IMAGE_IMPORT_DESCRIPTOR>(
            Test.GetBase());
          
          // Ensure ImportDir getters/setters 'match' by checking that the 
          // data is unchanged
          BOOST_CHECK_EQUAL(std::memcmp(&ImpDirRaw, &ImpDirRawNew, sizeof(
            IMAGE_IMPORT_DESCRIPTOR)), 0);
          
          // Enumerate import thunks for import dir
          HadesMem::ImportThunkList ImportThunks(MyPeFile, 
            D.GetCharacteristics());
          BOOST_CHECK(ImportThunks.begin() != ImportThunks.end());
          std::for_each(ImportThunks.begin(), ImportThunks.end(), 
            [&] (HadesMem::ImportThunk const& T)
            {
              // Test ImportThunk constructor
              HadesMem::ImportThunk const TestNew(MyPeFile, T.GetBase());
              
              // Get raw import thunk data
              auto const ImpThunkRaw = MyMemory.Read<IMAGE_THUNK_DATA>(
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
              auto const ImpThunkRawNew = MyMemory.Read<IMAGE_THUNK_DATA>(
                TestNew.GetBase());
                
              // Ensure ImportThunk getters/setters 'match' by checking that 
              // the data is unchanged
              BOOST_CHECK_EQUAL(std::memcmp(&ImpThunkRaw, &ImpThunkRawNew, 
                sizeof(ImpThunkRaw)), 0);
            });
        });
    });
}

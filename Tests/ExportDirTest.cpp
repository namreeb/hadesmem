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
#include <HadesMemory/PeLib/ExportDir.hpp>
#include <HadesMemory/PeLib/DosHeader.hpp>
#include <HadesMemory/PeLib/NtHeaders.hpp>

// Boost
#define BOOST_TEST_MODULE ExportDirTest
#include <boost/config.hpp>
#include <boost/test/unit_test.hpp>

// Export a function to ensure that at least one module has an export dir
BOOST_SYMBOL_EXPORT void TestExport() { }

// ExportDir component tests
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
        
      // Ensure module has an export directory before continuing
      // Todo: Ensure via test that at least one module with an export dir is 
      // processed (i.e. this one)
      Hades::Memory::ExportDir MyExportDir(MyPeFile);
      if (!MyExportDir.IsValid())
      {
        return;
      }
      
      // Get raw export dir data
      auto ExpDirRaw = MyMemory.Read<IMAGE_EXPORT_DIRECTORY>(
        MyExportDir.GetBase());
      
      // Ensure all member functions are called without exception, and 
      // overwrite the value of each field with the existing value
      BOOST_CHECK_EQUAL(MyExportDir.IsValid(), true);
      MyExportDir.EnsureValid();
      MyExportDir.SetCharacteristics(MyExportDir.GetCharacteristics());
      MyExportDir.SetTimeDateStamp(MyExportDir.GetTimeDateStamp());
      MyExportDir.SetMajorVersion(MyExportDir.GetMajorVersion());
      MyExportDir.SetMinorVersion(MyExportDir.GetMinorVersion());
      MyExportDir.SetOrdinalBase(MyExportDir.GetOrdinalBase());
      MyExportDir.SetNumberOfFunctions(MyExportDir.GetNumberOfFunctions());
      MyExportDir.SetNumberOfNames(MyExportDir.GetNumberOfNames());
      MyExportDir.SetAddressOfFunctions(MyExportDir.GetAddressOfFunctions());
      MyExportDir.SetAddressOfNames(MyExportDir.GetAddressOfNames());
      MyExportDir.SetAddressOfNameOrdinals(MyExportDir.
        GetAddressOfNameOrdinals());
      MyExportDir.SetCharacteristics(MyExportDir.GetCharacteristics());
      BOOST_CHECK(!MyExportDir.GetName().empty());
      MyExportDir.GetExportDirRaw();
      
      // Get raw export dir data again (using the member function this time)
      auto ExpDirRawNew = MyMemory.Read<IMAGE_EXPORT_DIRECTORY>(
        MyExportDir.GetBase());
        
      // Ensure ExportDir getters/setters 'match' by checking that the data is 
      // unchanged
      BOOST_CHECK_EQUAL(std::memcmp(&ExpDirRaw, &ExpDirRawNew, sizeof(
        ExpDirRaw)), 0);
        
      // Enumerate exports for module
      // Todo: Test export enumeration APIs before relying on them.
      // Todo: Ensure at least one export is processed in cases where we can 
      // prove it should.
      Hades::Memory::ExportList Exports(MyPeFile);
      std::for_each(Exports.begin(), Exports.end(), 
        [&] (Hades::Memory::Export& E)
        {
          // Test export constructor
          Hades::Memory::Export const Test(MyPeFile, E.GetOrdinal());
          
          // Ensure name/ordinal data is valid
          // Todo: Ensure Export::ByName works
          if (Test.ByName())
          {
            BOOST_CHECK(!Test.GetName().empty());
          }
          else
          {
            BOOST_CHECK(Test.GetOrdinal() >= MyExportDir.GetOrdinalBase());
          }
          
          // Ensure export forwarding data or RVA/VA data is valid
          // Todo: Ensure Export::Forwarded works
          if (Test.Forwarded())
          {
            BOOST_CHECK(!Test.GetForwarder().empty());
            BOOST_CHECK(!Test.GetForwarderModule().empty());
            BOOST_CHECK(!Test.GetForwarderFunction().empty());
          }
          else
          {
            BOOST_CHECK(Test.GetRva() != 0);
            BOOST_CHECK(Test.GetVa() != nullptr);
          }
        });
    });
}

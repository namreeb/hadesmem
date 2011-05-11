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

#define BOOST_TEST_MODULE ExportDirTest
#include <boost/test/unit_test.hpp>

#include "HadesMemory/ExportDir.hpp"
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
        
      Hades::Memory::ExportDir MyExportDir(MyPeFile);
      if (!MyExportDir.IsValid())
      {
        return;
      }
      
      auto ExpDirRaw = MyMemory.Read<IMAGE_EXPORT_DIRECTORY>(
        MyExportDir.GetBase());
      
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
      MyExportDir.SetAddressOfNameOrdinals(MyExportDir.GetAddressOfNameOrdinals());
      MyExportDir.SetCharacteristics(MyExportDir.GetCharacteristics());
      BOOST_CHECK(!MyExportDir.GetName().empty());
      MyExportDir.GetExportDirRaw();
      
      auto ExpDirRawNew = MyMemory.Read<IMAGE_EXPORT_DIRECTORY>(
        MyExportDir.GetBase());
        
      BOOST_CHECK_EQUAL(std::memcmp(&ExpDirRaw, &ExpDirRawNew, sizeof(
        IMAGE_EXPORT_DIRECTORY)), 0);
        
      Hades::Memory::ExportList Exports(MyPeFile);
      std::for_each(Exports.begin(), Exports.end(), 
        [&] (Hades::Memory::Export& E)
        {
          Hades::Memory::Export const Test(MyPeFile, E.GetOrdinal());
            
          if (Test.ByName())
          {
            BOOST_CHECK(!Test.GetName().empty());
          }
          else
          {
            BOOST_CHECK(Test.GetOrdinal() >= MyExportDir.GetOrdinalBase());
          }
          
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

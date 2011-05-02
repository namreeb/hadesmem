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

#define BOOST_TEST_MODULE TlsDirTest
#pragma warning(push, 1)
#include <boost/test/unit_test.hpp>
#pragma warning(pop)

#include "HadesMemory/TlsDir.hpp"
#include "HadesMemory/Module.hpp"
#include "HadesMemory/ModuleEnum.hpp"
#include "HadesMemory/PeFile.hpp"
#include "HadesMemory/MemoryMgr.hpp"

BOOST_AUTO_TEST_CASE(BOOST_TEST_MODULE)
{
  Hades::Memory::MemoryMgr MyMemory(GetCurrentProcessId());
    
  for (Hades::Memory::ModuleIter ModIter(MyMemory); *ModIter; ++ModIter)
  {
    Hades::Memory::Module const Mod = **ModIter;
      
    // Todo: Also test FileType_Data
    Hades::Memory::PeFile MyPeFile(MyMemory, Mod.GetBase());
      
    Hades::Memory::TlsDir MyTlsDir(MyPeFile);
    if (!MyTlsDir.IsValid())
    {
      continue;
    }
    
    auto TlsDirRaw = MyMemory.Read<IMAGE_TLS_DIRECTORY>(MyTlsDir.GetBase());
    
    BOOST_CHECK_EQUAL(MyTlsDir.IsValid(), true);
    MyTlsDir.EnsureValid();
    MyTlsDir.SetStartAddressOfRawData(MyTlsDir.GetStartAddressOfRawData());
    MyTlsDir.SetEndAddressOfRawData(MyTlsDir.GetEndAddressOfRawData());
    MyTlsDir.SetAddressOfIndex(MyTlsDir.GetAddressOfIndex());
    MyTlsDir.SetAddressOfCallBacks(MyTlsDir.GetAddressOfCallBacks());
    MyTlsDir.SetSizeOfZeroFill(MyTlsDir.GetSizeOfZeroFill());
    MyTlsDir.SetCharacteristics(MyTlsDir.GetCharacteristics());
    MyTlsDir.GetCallbacks();
    MyTlsDir.GetTlsDirRaw();
    
    auto TlsDirRawNew = MyMemory.Read<IMAGE_TLS_DIRECTORY>(
      MyTlsDir.GetBase());
      
    BOOST_CHECK_EQUAL(std::memcmp(&TlsDirRaw, &TlsDirRawNew, sizeof(
      IMAGE_TLS_DIRECTORY)), 0);
  }
}

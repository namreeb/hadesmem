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
#include "HadesCommon/Config.hpp"
#include "HadesMemory/Module.hpp"
#include "HadesMemory/MemoryMgr.hpp"
#include "HadesMemory/PeLib/TlsDir.hpp"
#include "HadesMemory/PeLib/PeFile.hpp"
#include "HadesMemory/PeLib/DosHeader.hpp"
#include "HadesMemory/PeLib/NtHeaders.hpp"

// C++ Standard Library
#include <algorithm>

// Boost
#define BOOST_TEST_MODULE TlsDirTest
#ifdef HADES_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
#include <boost/thread.hpp>
#ifdef HADES_GCC
#pragma GCC diagnostic pop
#endif
#include <boost/test/unit_test.hpp>
// TLS component tests
BOOST_AUTO_TEST_CASE(BOOST_TEST_MODULE)
{
  // Use threads and TSS to ensure that at least one module has a TLS dir
  auto DoNothing = [] () { };
  boost::thread DoNothingThread(DoNothing);
  boost::thread_specific_ptr<int> TssDummy;
  TssDummy.reset(new int(1234));
  
  // Create memory manager for self
  Hades::Memory::MemoryMgr MyMemory(GetCurrentProcessId());
    
  // Enumerate module list and run TLS dir tests on all modules
  Hades::Memory::ModuleList Modules(MyMemory);
  std::for_each(Modules.begin(), Modules.end(), 
    [&] (Hades::Memory::Module const& Mod) 
    {
      // Open module as a memory-based PeFile
      // Todo: Also test FileType_Data
      Hades::Memory::PeFile MyPeFile(MyMemory, Mod.GetBase());
      Hades::Memory::DosHeader const MyDosHeader(MyPeFile);
      Hades::Memory::NtHeaders const MyNtHeaders(MyPeFile);
      
      // Ensure module has a TLS directory before continuing
      // Todo: Ensure via test that at least one module with a TLS dir is 
      // processed (i.e. this one)
      Hades::Memory::TlsDir MyTlsDir(MyPeFile);
      if (!MyTlsDir.IsValid())
      {
        return;
      }
      
      // Get raw TLS dir data
      auto TlsDirRaw = MyMemory.Read<IMAGE_TLS_DIRECTORY>(MyTlsDir.GetBase());
      
      // Ensure all member functions are called without exception, and 
      // overwrite the value of each field with the existing value
      MyTlsDir.EnsureValid();
      MyTlsDir.SetStartAddressOfRawData(MyTlsDir.GetStartAddressOfRawData());
      MyTlsDir.SetEndAddressOfRawData(MyTlsDir.GetEndAddressOfRawData());
      MyTlsDir.SetAddressOfIndex(MyTlsDir.GetAddressOfIndex());
      MyTlsDir.SetAddressOfCallBacks(MyTlsDir.GetAddressOfCallBacks());
      MyTlsDir.SetSizeOfZeroFill(MyTlsDir.GetSizeOfZeroFill());
      MyTlsDir.SetCharacteristics(MyTlsDir.GetCharacteristics());
      MyTlsDir.GetCallbacks();
      MyTlsDir.GetTlsDirRaw();
      
      // Get raw TLS dir data again (using the member function this time)
      auto TlsDirRawNew = MyTlsDir.GetTlsDirRaw();
      
      // Ensure TlsDir getters/setters 'match' by checking that the data is 
      // unchanged
      BOOST_CHECK_EQUAL(std::memcmp(&TlsDirRaw, &TlsDirRawNew, sizeof(
        TlsDirRaw)), 0);
    });
}

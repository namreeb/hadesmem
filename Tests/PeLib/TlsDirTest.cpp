// Copyright Joshua Boyce 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

// Hades
#include <HadesMemory/Module.hpp>
#include <HadesMemory/MemoryMgr.hpp>
#include <HadesMemory/PeLib/TlsDir.hpp>
#include <HadesMemory/PeLib/PeFile.hpp>
#include <HadesMemory/Detail/Config.hpp>
#include <HadesMemory/PeLib/DosHeader.hpp>
#include <HadesMemory/PeLib/NtHeaders.hpp>

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

BOOST_AUTO_TEST_CASE(ConstructorsTest)
{
  // Create memory manager for self
  HadesMem::MemoryMgr MyMemory(GetCurrentProcessId());
    
  // Create PeFile
  HadesMem::PeFile MyPeFile(MyMemory, GetModuleHandle(NULL));
    
  // Create TLS dir
  HadesMem::TlsDir MyTlsDir(MyPeFile);
  
  // Test copying, assignement, and moving
  HadesMem::TlsDir OtherTlsDir(MyTlsDir);
  BOOST_CHECK(MyTlsDir == OtherTlsDir);
  MyTlsDir = OtherTlsDir;
  BOOST_CHECK(MyTlsDir == OtherTlsDir);
  HadesMem::TlsDir MovedTlsDir(std::move(OtherTlsDir));
  BOOST_CHECK(MovedTlsDir == MyTlsDir);
  HadesMem::TlsDir NewTestTlsDir(MyTlsDir);
  MyTlsDir = std::move(NewTestTlsDir);
  BOOST_CHECK(MyTlsDir == MovedTlsDir);
}

BOOST_AUTO_TEST_CASE(DataTest)
{
  // Use threads and TSS to ensure that at least one module has a TLS dir
  auto const DoNothing = [] () { };
  boost::thread DoNothingThread(DoNothing);
  boost::thread_specific_ptr<int> TssDummy;
  TssDummy.reset(new int(1234));
  
  // Create memory manager for self
  HadesMem::MemoryMgr const MyMemory(GetCurrentProcessId());
    
  // Enumerate module list and run TLS dir tests on all modules
  HadesMem::ModuleList Modules(MyMemory);
  std::for_each(Modules.begin(), Modules.end(), 
    [&] (HadesMem::Module const& Mod) 
    {
      // Open module as a memory-based PeFile
      // Todo: Also test FileType_Data
      HadesMem::PeFile const MyPeFile(MyMemory, Mod.GetHandle());
      HadesMem::DosHeader const MyDosHeader(MyPeFile);
      HadesMem::NtHeaders const MyNtHeaders(MyPeFile);
      
      // Create TLS dir
      HadesMem::TlsDir MyTlsDir(MyPeFile);
        
      // Check self
      if (Mod.GetHandle() == GetModuleHandle(NULL))
      {
        BOOST_CHECK(MyTlsDir.IsValid());
      }
      
      // Ensure module has a TLS directory before continuing
      if (!MyTlsDir.IsValid())
      {
        return;
      }
      
      // Get raw TLS dir data
      auto const TlsDirRaw = MyMemory.Read<IMAGE_TLS_DIRECTORY>(MyTlsDir.
        GetBase());
      
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
      
      // Get raw TLS dir data again
      auto const TlsDirRawNew = MyMemory.Read<IMAGE_TLS_DIRECTORY>(MyTlsDir.
        GetBase());
      
      // Ensure TlsDir getters/setters 'match' by checking that the data is 
      // unchanged
      BOOST_CHECK_EQUAL(std::memcmp(&TlsDirRaw, &TlsDirRawNew, sizeof(
        TlsDirRaw)), 0);
    });
}

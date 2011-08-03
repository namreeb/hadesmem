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
#include <HadesMemory/Injector.hpp>
#include <HadesMemory/MemoryMgr.hpp>

// C++ Standard Library
#include <utility>

// Boost
#define BOOST_TEST_MODULE InjectorTest
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(ConstructorsTest)
{
  // Create memory manager for self
  HadesMem::MemoryMgr MyMemory(GetCurrentProcessId());
    
  // Create injector
  HadesMem::Injector MyInjector(MyMemory);
      
  // Test copying, assignement, and moving
  HadesMem::Injector MyOtherInjector(MyInjector);
  BOOST_CHECK(MyInjector == MyOtherInjector);
  MyInjector = MyOtherInjector;
  BOOST_CHECK(MyInjector == MyOtherInjector);
  HadesMem::Injector MovedInjector(std::move(MyOtherInjector));
  BOOST_CHECK(MovedInjector == MyInjector);
  HadesMem::Injector NewTestInjector(MyInjector);
  MyInjector = std::move(NewTestInjector);
  BOOST_CHECK(MyInjector == MovedInjector);
}

BOOST_AUTO_TEST_CASE(InjectionTest)
{
  // Create memory manager for self
  HadesMem::MemoryMgr const MyMemory(GetCurrentProcessId());
    
  // Create injector manager
  HadesMem::Injector const MyInjector(MyMemory);
    
  // Get handle to Kernel32
  HMODULE const Kernel32Mod = GetModuleHandle(L"kernel32.dll");
  BOOST_REQUIRE(Kernel32Mod != nullptr);
  
  // 'Inject' Kernel32 with path resolution disabled and ensure that the 
  // module handle matches the one retrieved via GetModuleHandle earlier
  HMODULE const Kernel32ModNew = MyInjector.InjectDll(L"kernel32.dll", 
    HadesMem::Injector::InjectFlag_None);
  BOOST_CHECK_EQUAL(Kernel32Mod, Kernel32ModNew);
  
  // Todo: Test path resolution
  
  // Call Kernel32.dll!FreeLibrary and ensure the return value and 
  // last error code are their expected values
  HadesMem::MemoryMgr::RemoteFunctionRet const ExpRet = 
    MyInjector.CallExport(Kernel32ModNew, "FreeLibrary");
  BOOST_CHECK(ExpRet.GetReturnValue() != 0);
  BOOST_CHECK_EQUAL(ExpRet.GetLastError(), static_cast<DWORD>(0));
  
  // Perform injection test again so we can test the FreeDll API
  HMODULE const Kernel32ModNew2 = MyInjector.InjectDll(L"kernel32.dll", 
    HadesMem::Injector::InjectFlag_None);
  BOOST_CHECK_EQUAL(Kernel32Mod, Kernel32ModNew2);
  
  // Free kernel32.dll in remote process
  MyInjector.FreeDll(Kernel32ModNew2);
  
  // Todo: Test CreateAndInject API
}

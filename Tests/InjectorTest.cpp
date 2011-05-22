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

// Injector component tests
BOOST_AUTO_TEST_CASE(BOOST_TEST_MODULE)
{
  // Create memory manager for self
  Hades::Memory::MemoryMgr const MyMemory(GetCurrentProcessId());
    
  // Create injector manager
  Hades::Memory::Injector const MyInjector(MyMemory);
    
  // Get handle to Kernel32
  HMODULE const Kernel32Mod = GetModuleHandle(L"kernel32.dll");
  BOOST_REQUIRE(Kernel32Mod != nullptr);
  
  // 'Inject' Kernel32 with path resolution disabled and ensure that the 
  // module handle matches the one retrieved via GetModuleHandle earlier
  // Todo: Test path resolution
  HMODULE const Kernel32ModNew = MyInjector.InjectDll(L"kernel32.dll", false);
  BOOST_CHECK_EQUAL(Kernel32Mod, Kernel32ModNew);
  
  // Call Kernel32.dll!GetCurrentProcessId and ensure the return value and 
  // last error code are their expected values
  std::pair<DWORD_PTR, DWORD> const ExpRet = MyInjector.CallExport(
    L"kernel32.dll", Kernel32ModNew, "GetCurrentProcessId");
  BOOST_CHECK_EQUAL(ExpRet.first, GetCurrentProcessId());
  BOOST_CHECK_EQUAL(ExpRet.second, static_cast<DWORD>(0));
  
  // Todo: Test CreateAndInject API
}

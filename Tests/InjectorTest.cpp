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

#define BOOST_TEST_MODULE InjectorTest
#pragma warning(push, 1)
#include <boost/test/unit_test.hpp>
#pragma warning(pop)

#include <algorithm>

#include "HadesMemory/Memory.hpp"

BOOST_AUTO_TEST_CASE(InjectorTest)
{
  Hades::Memory::MemoryMgr MyMemory(GetCurrentProcessId());
    
  Hades::Memory::Injector MyInjector(MyMemory);
    
  HMODULE NtdllMod = GetModuleHandle(L"kernel32.dll");
  BOOST_REQUIRE(NtdllMod != nullptr);
    
  HMODULE NtdllModNew = MyInjector.InjectDll(L"kernel32.dll", false);
  BOOST_CHECK_EQUAL(NtdllMod, NtdllModNew);
  
  std::pair<DWORD_PTR, DWORD> ExpRet = MyInjector.CallExport(L"kernel32.dll", NtdllModNew, "GetCurrentProcessId");
  BOOST_CHECK_EQUAL(ExpRet.first, GetCurrentProcessId());
  BOOST_CHECK_EQUAL(ExpRet.second, 0);
  
  // Todo: Test CreateAndInject
}

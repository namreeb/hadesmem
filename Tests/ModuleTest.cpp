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

#define BOOST_TEST_MODULE ModuleTest
#include <boost/test/unit_test.hpp>

#include "HadesMemory/Module.hpp"
#include "HadesMemory/MemoryMgr.hpp"

BOOST_AUTO_TEST_CASE(BOOST_TEST_MODULE)
{
  Hades::Memory::MemoryMgr MyMemory(GetCurrentProcessId());
    
  Hades::Memory::ModuleList Modules(MyMemory);
  std::for_each(Modules.begin(), Modules.end(), 
    [&] (Hades::Memory::Module& M)
    {
      BOOST_CHECK(M.GetBase() != 0);
      BOOST_CHECK(M.GetSize() != 0);
      BOOST_CHECK(!M.GetName().empty());
      BOOST_CHECK(!M.GetPath().empty());
      
      Hades::Memory::Module TestMod1(MyMemory, M.GetBase());
      Hades::Memory::Module TestMod2(MyMemory, M.GetName());
      Hades::Memory::Module TestMod3(MyMemory, M.GetPath().wstring());
    });
}

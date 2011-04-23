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
#pragma warning(push, 1)
#include <boost/test/unit_test.hpp>
#pragma warning(pop)

#include "HadesMemory/Memory.hpp"

BOOST_AUTO_TEST_CASE(BOOST_TEST_MODULE)
{
  Hades::Memory::MemoryMgr MyMemory(GetCurrentProcessId());
   
  for (Hades::Memory::ModuleIter ModIter(MyMemory); *ModIter; ++ModIter)
  {
    Hades::Memory::Module const Mod = **ModIter;
    BOOST_CHECK(Mod.GetBase() != 0);
    BOOST_CHECK(Mod.GetSize() != 0);
    BOOST_CHECK(!Mod.GetName().empty());
    BOOST_CHECK(!Mod.GetPath().empty());
    
    Hades::Memory::Module TestMod1(MyMemory, Mod.GetBase());
    Hades::Memory::Module TestMod2(MyMemory, Mod.GetName());
    Hades::Memory::Module TestMod3(MyMemory, Mod.GetPath().wstring());
  }
}

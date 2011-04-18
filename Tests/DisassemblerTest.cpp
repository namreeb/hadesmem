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

#define BOOST_TEST_MODULE DisassemblerTest
#pragma warning(push, 1)
#include <boost/test/unit_test.hpp>
#pragma warning(pop)

#include <algorithm>

#include "HadesMemory/Memory.hpp"

BOOST_AUTO_TEST_CASE(DisassemblerTest)
{
  Hades::Memory::MemoryMgr MyMemory(GetCurrentProcessId());
    
  Hades::Memory::PeFile MyPeFile(MyMemory, GetModuleHandle(NULL));
    
  Hades::Memory::NtHeaders MyNtHeaders(MyPeFile);
    
  auto EntryPointRva = MyNtHeaders.GetAddressOfEntryPoint();
  auto pEntryPoint = MyPeFile.RvaToVa(EntryPointRva);
  BOOST_REQUIRE(pEntryPoint != 0);
  
  Hades::Memory::Disassembler MyDisassembler(MyMemory);
    
  BOOST_CHECK_EQUAL(MyDisassembler.Disassemble(pEntryPoint, 1).size(), 1);
  BOOST_CHECK_EQUAL(MyDisassembler.Disassemble(pEntryPoint, 50).size(), 50);
  BOOST_CHECK_EQUAL(MyDisassembler.Disassemble(pEntryPoint, 500).size(), 500);
  
  BOOST_CHECK_EQUAL(MyDisassembler.DisassembleToStr(pEntryPoint, 1).size(), 1);
  BOOST_CHECK_EQUAL(MyDisassembler.DisassembleToStr(pEntryPoint, 50).size(), 
    50);
  BOOST_CHECK_EQUAL(MyDisassembler.DisassembleToStr(pEntryPoint, 500).size(), 
    500);
  
  auto DisasmData = MyDisassembler.Disassemble(pEntryPoint, 500);
  std::for_each(DisasmData.begin(), DisasmData.end(), 
    [] (Hades::Memory::DisasmData const& Data)
  {
    BOOST_CHECK_EQUAL(Data.Len, Data.Raw.size());
  });
  
  auto DisasmDataStr = MyDisassembler.DisassembleToStr(pEntryPoint, 500);
  std::for_each(DisasmDataStr.begin(), DisasmDataStr.end(), 
    [] (std::wstring const& Str)
  {
    BOOST_CHECK(!Str.empty());
  });
}

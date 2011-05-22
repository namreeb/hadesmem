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
#include <HadesMemory/MemoryMgr.hpp>
#include <HadesMemory/Disassembler.hpp>
#include <HadesMemory/PeLib/PeFile.hpp>
#include <HadesMemory/PeLib/DosHeader.hpp>
#include <HadesMemory/PeLib/NtHeaders.hpp>

// C++ Standard Library
#include <algorithm>

// Boost
#define BOOST_TEST_MODULE DisassemblerTest
#include <boost/test/unit_test.hpp>

// Disassembler component tests
BOOST_AUTO_TEST_CASE(BOOST_TEST_MODULE)
{
  // Create memory manager for self
  Hades::Memory::MemoryMgr const MyMemory(GetCurrentProcessId());

  // Open self as a memory-based PeFile
  Hades::Memory::PeFile const MyPeFile(MyMemory, GetModuleHandle(NULL));
  Hades::Memory::DosHeader const MyDosHeader(MyPeFile);
  Hades::Memory::NtHeaders const MyNtHeaders(MyPeFile);
    
  // Get EP of self from NT headers
  auto const EntryPointRva = MyNtHeaders.GetAddressOfEntryPoint();
  auto const pEntryPoint = MyPeFile.RvaToVa(EntryPointRva);
  BOOST_REQUIRE(pEntryPoint != 0);
  
  // Create disasembler manager
  Hades::Memory::Disassembler const MyDisassembler(MyMemory);
  
  // Disassemble EP for various lengths and ensure it succeeded
  BOOST_CHECK_EQUAL(MyDisassembler.Disassemble(pEntryPoint, 1).size(), 
    static_cast<std::size_t>(1));
  BOOST_CHECK_EQUAL(MyDisassembler.Disassemble(pEntryPoint, 50).size(), 
    static_cast<std::size_t>(50));
  BOOST_CHECK_EQUAL(MyDisassembler.Disassemble(pEntryPoint, 500).size(), 
    static_cast<std::size_t>(500));
  
  // Disassemble EP to strings for various lengths and ensure it succeeded
  BOOST_CHECK_EQUAL(MyDisassembler.DisassembleToStr(pEntryPoint, 1).size(), 
    static_cast<std::size_t>(1));
  BOOST_CHECK_EQUAL(MyDisassembler.DisassembleToStr(pEntryPoint, 50).size(), 
    static_cast<std::size_t>(50));
  BOOST_CHECK_EQUAL(MyDisassembler.DisassembleToStr(pEntryPoint, 500).size(), 
    static_cast<std::size_t>(500));
  
  // Verify that the raw data buffer in the DiasmData structure matches the 
  // specified length.
  auto const DisasmData = MyDisassembler.Disassemble(pEntryPoint, 500);
  std::for_each(DisasmData.begin(), DisasmData.end(), 
    [] (Hades::Memory::DisasmData const& Data)
  {
    BOOST_CHECK_EQUAL(static_cast<std::size_t>(Data.Len), Data.Raw.size());
  });
  
  // Verify that none of the instruction strings are empty
  auto const DisasmDataStr = MyDisassembler.DisassembleToStr(pEntryPoint, 
    500);
  std::for_each(DisasmDataStr.begin(), DisasmDataStr.end(), 
    [] (std::wstring const& Str)
  {
    BOOST_CHECK(!Str.empty());
  });
}

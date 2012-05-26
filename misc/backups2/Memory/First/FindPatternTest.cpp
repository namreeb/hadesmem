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
#include <HadesMemory/FindPattern.hpp>
#include <HadesMemory/MemoryMgr.hpp>

// Boost
#define BOOST_TEST_MODULE FindPatternTest
#include <boost/test/unit_test.hpp>

// FindPattern component tests
BOOST_AUTO_TEST_CASE(BOOST_TEST_MODULE)
{
  // Create memory manager for self
  Hades::Memory::MemoryMgr const MyMemory(GetCurrentProcessId());
    
  // Get base of self
  HMODULE hSelf = GetModuleHandle(NULL);
  BOOST_REQUIRE(hSelf != nullptr);
  DWORD_PTR pSelf = reinterpret_cast<DWORD_PTR>(hSelf);
    
  // Create pattern scanner targetting self
  Hades::Memory::FindPattern MyFindPattern(MyMemory, hSelf);
    
  // Create pattern scanner targetting self (using default constructor this 
  // time)
  MyFindPattern = Hades::Memory::FindPattern(MyMemory);
    
  // Ensure constructor throws if an invalid module handle is specified
  BOOST_CHECK_THROW(MyFindPattern = Hades::Memory::FindPattern(MyMemory, 
    reinterpret_cast<HMODULE>(-1)), Hades::HadesError);
  
  // Scan for predicatable byte mask
  auto const pNop = MyFindPattern.Find(L"90");
  // Ensure pattern was found
  BOOST_CHECK(pNop != nullptr);
  // Ensure pattern address is valid
  BOOST_CHECK_GT(pNop, hSelf);
  // Scan again for same pattern, this time specifying a name
  MyFindPattern.Find(L"90", L"Nop");
  // Ensure pattern is found, is added to the named map, and is equal to the 
  // previously found instance
  BOOST_CHECK_EQUAL(pNop, MyFindPattern[L"Nop"]);
  // Ensure named map is the expected size
  BOOST_CHECK_EQUAL(MyFindPattern.GetAddresses().size(), 
    static_cast<std::size_t>(1));
  // Scan again for same pattern, this time specifying the relative address 
  // flag
  auto const pNopRel = MyFindPattern.Find(L"90", 
    Hades::Memory::FindPattern::FindFlags_RelativeAddress);
  // Ensure the relative address is correct
  BOOST_CHECK_EQUAL(static_cast<PBYTE>(pNopRel) + pSelf, pNop);  
  // Test stream-based pattern scanner by scanning for the same pattern, with 
  // a different name
  Hades::Memory::Pattern NopPattern(MyFindPattern, L"90", L"NopPlus1");
  // Apply 'Add' manipulator to pattern and save back to parent
  NopPattern << Hades::Memory::PatternManipulators::Add(1) << 
    Hades::Memory::PatternManipulators::Save();
  // Ensure manipulator was correctly applied
  BOOST_CHECK_EQUAL(NopPattern.GetAddress(), static_cast<PBYTE>(pNop) + 1);
  // Ensure pattern result was saved back to parent
  BOOST_CHECK_EQUAL(NopPattern.GetAddress(), MyFindPattern[L"NopPlus1"]);
  // Ensure named map is the expected size
  BOOST_CHECK_EQUAL(MyFindPattern.GetAddresses().size(), 
    static_cast<std::size_t>(2));
  
  // Scan for predictable byte mask (including wildcard)
  auto const pZeros = MyFindPattern.Find(L"00 ?? 00");
  // Ensure pattern was found
  BOOST_CHECK(pZeros != nullptr);
  // Ensure pattern address is valid
  BOOST_CHECK_GT(pZeros, GetModuleHandle(NULL));
  // Scan again for same pattern, this time specifying a name
  MyFindPattern.Find(L"00 ?? 00", L"Zeros");
  // Ensure pattern is found, is added to the named map, and is equal to the 
  // previously found instance
  BOOST_CHECK_EQUAL(pZeros, MyFindPattern[L"Zeros"]);
  // Ensure named map is the expected size
  BOOST_CHECK_EQUAL(MyFindPattern.GetAddresses().size(), 
    static_cast<std::size_t>(3));
  // Ensure this pattern (00 ?? 00) does not match the earlier pattern (90)
  BOOST_CHECK(pNop != pZeros);
  // Scan again for same pattern, this time specifying the relative address 
  // flag
  auto const pZerosRel = MyFindPattern.Find(L"00 ?? 00", 
    Hades::Memory::FindPattern::FindFlags_RelativeAddress);
  // Ensure the relative address is correct
  BOOST_CHECK_EQUAL(static_cast<PBYTE>(pZerosRel) + pSelf, pZeros);
  // Test stream-based pattern scanner by scanning for the same pattern, with 
  // a different name  
  Hades::Memory::Pattern ZerosPattern(MyFindPattern, L"00 ?? 00", 
    L"ZerosMinus1");
  // Apply 'Sub' manipulator to pattern and save back to parent
  ZerosPattern << Hades::Memory::PatternManipulators::Sub(1) << 
    Hades::Memory::PatternManipulators::Save();
  // Ensure manipulator was correctly applied
  BOOST_CHECK_EQUAL(ZerosPattern.GetAddress(), static_cast<PBYTE>(pZeros) - 1);
  // Ensure pattern result was saved back to parent
  BOOST_CHECK_EQUAL(ZerosPattern.GetAddress(), MyFindPattern[L"ZerosMinus1"]);
  // Ensure named map is the expected size
  BOOST_CHECK_EQUAL(MyFindPattern.GetAddresses().size(), 
    static_cast<std::size_t>(4));
  
  // Test stream-based pattern scanner by scanning for an instruction with a 
  // known relative operand. We should ensure that we're not going to hit this 
  // particular byte accidently as part of the operand of another instruction, 
  // but for now lets just ignore that possibility. The test will (or should) 
  // fail in that case.
  Hades::Memory::Pattern CallPattern(MyFindPattern, L"E8", 
    Hades::Memory::FindPattern::FindFlags_RelativeAddress);
  // The instruction is a 'Call' instruction, so add one byte to move past 
  // the instruction, then perform a relative dereference using the instruction 
  // size and operand offset (5 and 1 respectively in this case, for a 32-bit 
  // relative call).
  CallPattern << Hades::Memory::PatternManipulators::Add(1) << 
    Hades::Memory::PatternManipulators::Rel(5, 1);
  // Ensure pattern was found
  BOOST_CHECK(CallPattern.GetAddress() != nullptr);
  
  // Todo: PatternManipulators::Lea test
 
  // Test pattern file, including scan flags, pattern matching, and optional 
  // manipulators. Use the same patterns we have been using already so we 
  // can check their validity.
  std::wstring const PatternFileData = 
    L"HadesMem Patterns (RelativeAddress, ThrowOnUnmatch)\n"
    L"{ First Call, E8 }\n"
    L"[ Add, 1 ]\n"
    L"[ Rel, 5, 1 ]\n"
    L"{ Zeros New, 00 ?? 00 }\n"
    L"[ Add, 1 ]\n"
    L"[ Sub, 1 ]\n"
    L"{ Nop Other, 90 }\n";
  MyFindPattern.LoadFileMemory(PatternFileData);
  // Ensure all patterns match previous scans
  BOOST_CHECK_EQUAL(MyFindPattern[L"First Call"], CallPattern.GetAddress());
  BOOST_CHECK_EQUAL(MyFindPattern[L"Zeros New"], pZerosRel);
  BOOST_CHECK_EQUAL(MyFindPattern[L"Nop Other"], pNopRel);
 
  // Test pattern file, using various types of invalid input.
  std::wstring const PatternFileDataInvalid1 = 
    L"HadesMem Patterns (InvalidFlag)\n";
  BOOST_CHECK_THROW(MyFindPattern.LoadFileMemory(PatternFileDataInvalid1), 
    Hades::Memory::FindPattern::Error);
  std::wstring const PatternFileDataInvalid2 = 
    L"HadesMem Patterns (RelativeAddress, ThrowOnUnmatch)\n"
    L"[ Add, 1 ]";
  BOOST_CHECK_THROW(MyFindPattern.LoadFileMemory(PatternFileDataInvalid2), 
    Hades::Memory::FindPattern::Error);
  std::wstring const PatternFileDataInvalid3 = 
    L"HadesMem Patterns (RelativeAddress, ThrowOnUnmatch)\n"
    L"{ Foo, ZZ }";
  BOOST_CHECK_THROW(MyFindPattern.LoadFileMemory(PatternFileDataInvalid3), 
    Hades::Memory::FindPattern::Error);
  std::wstring const PatternFileDataInvalid4 = 
    L"HadesMem Patterns (RelativeAddress, ThrowOnUnmatch)\n"
    L"{ }";
  BOOST_CHECK_THROW(MyFindPattern.LoadFileMemory(PatternFileDataInvalid4), 
    Hades::Memory::FindPattern::Error);
  
  // Todo: LoadFile test
  
  // Perform a full wildcard scan twice and ensure that both scans return the 
  // same address
  auto const pNopsAny = MyFindPattern.Find(L"?? ?? ?? ?? ??");
  auto const pInt3sAny = MyFindPattern.Find(L"?? ?? ?? ?? ??");
  BOOST_CHECK_EQUAL(pNopsAny, pInt3sAny);
  // Ensure address is valid
  BOOST_CHECK_GT(pNopsAny, GetModuleHandle(NULL));
  // Perform scan again with relative address flag
  auto const pNopsAnyRel = MyFindPattern.Find(L"?? ?? ?? ?? ??", 
    Hades::Memory::FindPattern::FindFlags_RelativeAddress);
  // Ensure address is valid
  BOOST_CHECK_EQUAL(static_cast<PBYTE>(pNopsAnyRel) + pSelf, pNopsAny);
  
  // Check ThrowOnUnmatch flag
  BOOST_CHECK_THROW(MyFindPattern.Find(L"AA BB CC DD EE FF 11 22 33 44 55 66 "
    L"77 88 99 00 11 33 33 77", Hades::Memory::FindPattern::
    FindFlags_ThrowOnUnmatch), Hades::Memory::FindPattern::Error);
  
  // Check ScanData flag
  // Note: Pattern is for narrow string 'FindPattern' (without quotes)
  auto const pFindPatternStr = MyFindPattern.Find(L"46 69 6E 64 50 61 74 74 "
    L"65 72 6E", Hades::Memory::FindPattern::FindFlags_ScanData);
  BOOST_CHECK(pFindPatternStr != nullptr);
  
  // Check conversion failures throw
  BOOST_CHECK_THROW(MyFindPattern.Find(L"ZZ"), 
    Hades::Memory::FindPattern::Error);
  
  // Check conversion failures throw
  BOOST_CHECK_THROW(MyFindPattern.Find(L""), 
    Hades::Memory::FindPattern::Error);
}

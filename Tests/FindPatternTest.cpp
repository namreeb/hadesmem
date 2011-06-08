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

// Constant which should be scannable with data scans
static DWORD const MagicValue = 0x77333311;

// FindPattern component tests
BOOST_AUTO_TEST_CASE(BOOST_TEST_MODULE)
{
  // Create memory manager for self
  Hades::Memory::MemoryMgr const MyMemory(GetCurrentProcessId());
    
  // Create pattern scanner targetting self
  Hades::Memory::FindPattern MyFindPattern(MyMemory, 
    GetModuleHandle(NULL));
  // Create pattern scanner targetting self (using default constructor this 
  // time)
  MyFindPattern = Hades::Memory::FindPattern(MyMemory);
    
  // Get base of self
  DWORD_PTR pSelf = reinterpret_cast<DWORD_PTR>(GetModuleHandle(NULL));
  
  // Scan for predicatable byte masks and ensure that they were found and are 
  // different.
  {
    auto const pNop = MyFindPattern.Find(L"90", L"x");
    BOOST_CHECK(pNop != nullptr);
    BOOST_CHECK_GT(pNop, GetModuleHandle(NULL));
    MyFindPattern.Find(L"90", L"x", L"Nop");
    BOOST_CHECK_EQUAL(pNop, MyFindPattern[L"Nop"]);
    BOOST_CHECK_EQUAL(MyFindPattern.GetAddresses().size(), 
      static_cast<std::size_t>(1));
    auto const pNopRel = MyFindPattern.Find(L"90", L"x", 
      Hades::Memory::FindPattern::RelativeAddress);
    BOOST_CHECK_EQUAL(static_cast<PBYTE>(pNopRel) + pSelf, pNop);
    
    Hades::Memory::Pattern NopPattern(MyFindPattern, L"90", L"x", L"NopPlus1");
    NopPattern << Hades::Memory::PatternManipulators::Add(1) << 
      Hades::Memory::PatternManipulators::Save();
    BOOST_CHECK_EQUAL(NopPattern.GetAddress(), static_cast<PBYTE>(pNop) + 1);
    BOOST_CHECK_EQUAL(NopPattern.GetAddress(), MyFindPattern[L"NopPlus1"]);
    BOOST_CHECK_EQUAL(MyFindPattern.GetAddresses().size(), 
      static_cast<std::size_t>(2));
    
    auto const pZeros = MyFindPattern.Find(L"00 00 00", L"x?x");
    BOOST_CHECK(pZeros != nullptr);
    BOOST_CHECK_GT(pZeros, GetModuleHandle(NULL));
    MyFindPattern.Find(L"00 00 00", L"x?x", L"Zeros");
    BOOST_CHECK_EQUAL(pZeros, MyFindPattern[L"Zeros"]);
    BOOST_CHECK_EQUAL(MyFindPattern.GetAddresses().size(), 
      static_cast<std::size_t>(3));
    BOOST_CHECK(pNop != pZeros);
    auto const pZerosRel = MyFindPattern.Find(L"00 00 00", L"x?x", 
      Hades::Memory::FindPattern::RelativeAddress);
    BOOST_CHECK_EQUAL(static_cast<PBYTE>(pZerosRel) + pSelf, pZeros);
    
    Hades::Memory::Pattern ZerosPattern(MyFindPattern, L"00 00 00", L"x?x", 
      L"ZerosMinus1");
    ZerosPattern << Hades::Memory::PatternManipulators::Sub(1) << 
      Hades::Memory::PatternManipulators::Save();
    BOOST_CHECK_EQUAL(ZerosPattern.GetAddress(), static_cast<PBYTE>(pZeros) - 1);
    BOOST_CHECK_EQUAL(ZerosPattern.GetAddress(), MyFindPattern[L"ZerosMinus1"]);
    BOOST_CHECK_EQUAL(MyFindPattern.GetAddresses().size(), 
      static_cast<std::size_t>(4));
    
    Hades::Memory::Pattern CallPattern(MyFindPattern, L"E8", L"x");
    CallPattern << Hades::Memory::PatternManipulators::Add(1) << 
      Hades::Memory::PatternManipulators::Rel(5, 1);
    BOOST_CHECK(CallPattern.GetAddress() != nullptr);
    
    // Todo: PatternManipulators::Lea test
  }
  
  // Perform a full wildcard scan and ensure that both scans return the same 
  // pointer despite different data.
  {
    auto const pNopsAny = MyFindPattern.Find(L"90 90 90 90 90", L"?????");
    BOOST_CHECK_GT(pNopsAny, GetModuleHandle(NULL));
    MyFindPattern.Find(L"90 90 90 90 90", L"?????", L"NopsAny");
    BOOST_CHECK_EQUAL(pNopsAny, MyFindPattern[L"NopsAny"]);
    BOOST_CHECK_EQUAL(MyFindPattern.GetAddresses().size(), 
      static_cast<std::size_t>(5));
    auto const pNopsAnyRel = MyFindPattern.Find(L"90 90 90 90 90", L"?????", 
      Hades::Memory::FindPattern::RelativeAddress);
    BOOST_CHECK_EQUAL(static_cast<PBYTE>(pNopsAnyRel) + pSelf, pNopsAny);
    
    auto const pInt3sAny = MyFindPattern.Find(L"CC CC CC CC CC", L"?????");
    BOOST_CHECK_GT(pInt3sAny, GetModuleHandle(NULL));
    MyFindPattern.Find(L"CC CC CC CC CC", L"?????", L"Int3sAny");
    BOOST_CHECK_EQUAL(pInt3sAny, MyFindPattern[L"Int3sAny"]);
    BOOST_CHECK_EQUAL(MyFindPattern.GetAddresses().size(), 
      static_cast<std::size_t>(6));
    BOOST_CHECK_EQUAL(pNopsAny, pInt3sAny);
    auto const pInt3sAnyRel = MyFindPattern.Find(L"CC CC CC CC CC", L"?????", 
      Hades::Memory::FindPattern::RelativeAddress);
    BOOST_CHECK_EQUAL(static_cast<PBYTE>(pInt3sAnyRel) + pSelf, pInt3sAny);
  }
  
  // Check ThrowOnUnmatch flag
  BOOST_CHECK_THROW(MyFindPattern.Find(L"AA BB CC DD EE FF 11 22 33 44 55 "
    L"66 77 88 99 00 11 33 33 77", L"xxxxxxxxxxxxxxxxxxxx", 
    Hades::Memory::FindPattern::ThrowOnUnmatch), 
    Hades::Memory::FindPattern::Error);
  
  // Check ScanData flag
  // Note: Pattern is for narrow string 'FindPattern' (without quotes)
  auto const pFindPatternStr = MyFindPattern.Find(L"46 69 6E 64 50 61 74 74 "
    L"65 72 6E", L"xxxxxxxxxxx", Hades::Memory::FindPattern::ScanData);
  BOOST_CHECK(pFindPatternStr != nullptr);
  
  // Check conversion failures throw
  BOOST_CHECK_THROW(MyFindPattern.Find(L"ZZ", L"x"), 
    Hades::Memory::FindPattern::Error);
}

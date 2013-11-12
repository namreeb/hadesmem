// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/find_pattern.hpp>

#define BOOST_TEST_MODULE find_pattern
#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/test/unit_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/detail/initialize.hpp>

// Boost.Test causes the following warning under GCC:
// error: base class 'struct boost::unit_test::ut_detail::nil_t' has a 
// non-virtual destructor [-Werror=effc++]
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic ignored "-Weffc++"
#endif // #if defined(HADESMEM_GCC)

// Boost.Test causes the following warning under Clang:
// error: declaration requires a global constructor 
// [-Werror,-Wglobal-constructors]
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#endif // #if defined(HADESMEM_CLANG)

// TODO: Clean up, expand, fix, etc these tests.
// TODO: Add more tests (e.g. stream overload tests).

BOOST_AUTO_TEST_CASE(initialize)
{
    hadesmem::detail::InitializeAll();
}

// Using an underscore to avoid a variable shadowing waring under Clang 
// and GCC.
// TODO: Fix this.
BOOST_AUTO_TEST_CASE(find_pattern_)
{
    hadesmem::Process const process(::GetCurrentProcessId());

    HMODULE const self = GetModuleHandle(nullptr);
    BOOST_REQUIRE(self != nullptr);

    hadesmem::FindPattern find_pattern(process, self);

    // Create pattern scanner targetting self (using default constructor this 
    // time)
    find_pattern = hadesmem::FindPattern(process, nullptr);
    // Ensure constructor throws if an invalid module handle is specified
    // TODO: Fix this.
    //BOOST_CHECK_THROW(find_pattern = hadesmem::FindPattern(process, 
    //  reinterpret_cast<HMODULE>(-1)), hadesmem::Error);

    // Scan for predicatable byte mask
    auto const nop = find_pattern.Find(L"90", 
        hadesmem::FindPatternFlags::kNone);
    // Ensure pattern was found
    BOOST_CHECK(nop != nullptr);
    // Ensure pattern address is valid
    BOOST_CHECK_GT(nop, self);
    // Scan again for same pattern, this time specifying a name
    find_pattern.Find(L"90", L"Nop", hadesmem::FindPatternFlags::kNone);
    // Ensure pattern is found, is added to the named map, and is equal to 
    // the previously found instance
    BOOST_CHECK_EQUAL(nop, find_pattern[L"Nop"]);
    // Ensure named map is the expected size
    BOOST_CHECK_EQUAL(find_pattern.GetAddresses().size(), 1UL);
    // Scan again for same pattern, this time specifying the relative address 
    // flag
    auto const nop_rel = find_pattern.Find(L"90",
        hadesmem::FindPatternFlags::kRelativeAddress);
    // Ensure the relative address is correct
    BOOST_CHECK_EQUAL(static_cast<PBYTE>(nop_rel)+
        reinterpret_cast<DWORD_PTR>(self), nop);
    // Test stream-based pattern scanner by scanning for the same pattern, 
    // with a different name
    hadesmem::Pattern nop_pattern(find_pattern, L"90", L"NopPlus1",
        hadesmem::FindPatternFlags::kNone);
    // Apply 'Add' manipulator to pattern and save back to parent
    nop_pattern << hadesmem::pattern_manipulators::Add(1) <<
        hadesmem::pattern_manipulators::Save();
    // Ensure manipulator was correctly applied
    BOOST_CHECK_EQUAL(nop_pattern.GetAddress(), static_cast<PBYTE>(nop)+1);
    // Ensure pattern result was saved back to parent
    BOOST_CHECK_EQUAL(nop_pattern.GetAddress(), find_pattern[L"NopPlus1"]);
    // Ensure named map is the expected size
    BOOST_CHECK_EQUAL(find_pattern.GetAddresses().size(), 2UL);

    // Scan for predictable byte mask (including wildcard)
    auto const zeros = find_pattern.Find(L"00 ?? 00",
        hadesmem::FindPatternFlags::kNone);
    // Ensure pattern was found
    BOOST_CHECK(zeros != nullptr);
    // Ensure pattern address is valid
    BOOST_CHECK_GT(zeros, GetModuleHandle(nullptr));
    // Scan again for same pattern, this time specifying a name
    find_pattern.Find(L"00 ?? 00", L"Zeros",
        hadesmem::FindPatternFlags::kNone);
    // Ensure pattern is found, is added to the named map, and is equal to 
    // the previously found instance
    BOOST_CHECK_EQUAL(zeros, find_pattern[L"Zeros"]);
    // Ensure named map is the expected size
    BOOST_CHECK_EQUAL(find_pattern.GetAddresses().size(), 3UL);
    // Ensure this pattern (00 ?? 00) does not match the earlier pattern (90) 
    BOOST_CHECK(nop != zeros);
    // Scan again for same pattern, this time specifying the relative address 
    // flag
    auto const zeros_rel = find_pattern.Find(L"00 ?? 00",
        hadesmem::FindPatternFlags::kRelativeAddress);
    // Ensure the relative address is correct
    BOOST_CHECK_EQUAL(static_cast<PBYTE>(zeros_rel)+
        reinterpret_cast<DWORD_PTR>(self), zeros);
    // Test stream-based pattern scanner by scanning for the same pattern, 
    // with a different name  
    hadesmem::Pattern zeros_pattern(find_pattern, L"00 ?? 00", 
        L"ZerosMinus1", hadesmem::FindPatternFlags::kNone);
    // Apply 'Sub' manipulator to pattern and save back to parent
    zeros_pattern << hadesmem::pattern_manipulators::Sub(1) <<
        hadesmem::pattern_manipulators::Save();
    // Ensure manipulator was correctly applied
    BOOST_CHECK_EQUAL(static_cast<PVOID>(zeros_pattern.GetAddress()), 
        static_cast<PVOID>(static_cast<PBYTE>(zeros)-1));
    // Ensure pattern result was saved back to parent
    BOOST_CHECK_EQUAL(zeros_pattern.GetAddress(), 
        find_pattern[L"ZerosMinus1"]);
    // Ensure named map is the expected size
    BOOST_CHECK_EQUAL(find_pattern.GetAddresses().size(), 4UL);

    // Test stream-based pattern scanner by scanning for an instruction with 
    // a known relative operand. We should ensure that we're not going to hit 
    // this particular byte accidently as part of the operand of another 
    // instruction, but for now lets just ignore that possibility. The test 
    // will (or should) fail in that case.
    hadesmem::Pattern call_pattern(find_pattern, L"E8",
        hadesmem::FindPatternFlags::kRelativeAddress);
    // Ensure pattern was found
    BOOST_CHECK(call_pattern.GetAddress() != nullptr);
    // The instruction is a 'Call' instruction, so add one byte to move 
    // past the instruction, then perform a relative dereference using the 
    // instruction size and operand offset (5 and 1 respectively in this 
    // case, for a 32-bit relative call).
    call_pattern << hadesmem::pattern_manipulators::Add(1) <<
        hadesmem::pattern_manipulators::Rel(5, 1);
    // Ensure pattern was found
    BOOST_CHECK(call_pattern.GetAddress() != nullptr);

    // Todo: pattern_manipulators::Lea test

    // Test pattern file, including scan flags, pattern matching, and 
    // optional manipulators. Use the same patterns we have been using 
    // already so we can check their validity.
    std::wstring const pattern_file_data =
        LR"(
<?xml version="1.0" encoding="utf-8"?>
<HadesMem>
  <FindPattern>
    <Flag Name="RelativeAddress"/>
    <Flag Name="ThrowOnUnmatch"/>
    <Pattern Name="First Call" Data="E8">
      <Manipulator Name="Add" Operand1="1"></Manipulator>
      <Manipulator Name="Rel" Operand1="5" Operand2="1"></Manipulator>
    </Pattern>
    <Pattern Name="Zeros New" Data="00 ?? 00">
      <Manipulator Name="Add" Operand1="1"></Manipulator>
      <Manipulator Name="Sub" Operand1="1"></Manipulator>
    </Pattern>
    <Pattern Name="Nop Other" Data="90"/>
  </FindPattern>
</HadesMem>
)";
    find_pattern.LoadFileMemory(pattern_file_data);
    // Ensure all patterns match previous scans
    BOOST_CHECK_EQUAL(find_pattern.Lookup(L"First Call"), 
        call_pattern.GetAddress());
    BOOST_CHECK_EQUAL(find_pattern.Lookup(L"First Call"), 
        find_pattern[L"First Call"]);
    BOOST_CHECK_EQUAL(find_pattern.Lookup(L"Zeros New"), 
        zeros_rel);
    BOOST_CHECK_EQUAL(find_pattern.Lookup(L"Zeros New"), 
        find_pattern[L"Zeros New"]);
    BOOST_CHECK_EQUAL(find_pattern.Lookup(L"Nop Other"), 
        nop_rel);
    BOOST_CHECK_EQUAL(find_pattern.Lookup(L"Nop Other"), 
        find_pattern[L"Nop Other"]);

    // Test pattern file, using various types of invalid input.
    // TODO: Fix the test to ensure we get the error we're expecting, rather 
    // than just any error.
    std::wstring const pattern_file_data_invalid1 =
        LR"(
<?xml version="1.0" encoding="utf-8"?>
<HadesMem>
  <FindPattern>
    <Flag Name="InvalidFlag"></Flag>
  </FindPattern>
</HadesMem>
)";
    BOOST_CHECK_THROW(find_pattern.LoadFileMemory(
        pattern_file_data_invalid1), 
        hadesmem::Error);

    std::wstring const pattern_file_data_invalid2 =
        LR"(
<?xml version="1.0" encoding="utf-8"?>
<HadesMem>
  <FindPattern>
    <Flag Name="RelativeAddress"></Flag>
    <Flag Name="ThrowOnUnmatch"></Flag>
    <Pattern Name="Foo" Data="ZZ"></Pattern>
  </FindPattern>
</HadesMem>
)";
    BOOST_CHECK_THROW(find_pattern.LoadFileMemory(
        pattern_file_data_invalid2), 
        hadesmem::Error);

    std::wstring const pattern_file_data_invalid3 =
        LR"(
<?xml version="1.0" encoding="utf-8"?>
<HadesMem>
  <FindPattern>
    <Flag Name="RelativeAddress"></Flag>
    <Flag Name="ThrowOnUnmatch"></Flag>
    <Pattern></Pattern>
  </FindPattern>
</HadesMem>
)";
    BOOST_CHECK_THROW(find_pattern.LoadFileMemory(
        pattern_file_data_invalid3),
        hadesmem::Error);

    // Todo: LoadFile test

    // Perform a full wildcard scan twice and ensure that both scans return 
    // the same address
    auto const nops_any = find_pattern.Find(L"?? ?? ?? ?? ??",
        hadesmem::FindPatternFlags::kNone);
    auto const int3s_any = find_pattern.Find(L"?? ?? ?? ?? ??",
        hadesmem::FindPatternFlags::kNone);
    BOOST_CHECK_EQUAL(nops_any, int3s_any);
    // Ensure address is valid
    BOOST_CHECK_GT(nops_any, GetModuleHandle(nullptr));
    // Perform scan again with relative address flag
    auto const nops_any_rel = find_pattern.Find(L"?? ?? ?? ?? ??",
        hadesmem::FindPatternFlags::kRelativeAddress);
    // Ensure address is valid
    BOOST_CHECK_EQUAL(static_cast<PBYTE>(nops_any_rel)+
        reinterpret_cast<DWORD_PTR>(self), nops_any);

    // Check ThrowOnUnmatch flag
    BOOST_CHECK_THROW(find_pattern.Find(L"AA BB CC DD EE FF 11 22 33 44 55 "
        L"66 77 88 99 00 11 33 33 77", 
        hadesmem::FindPatternFlags::kThrowOnUnmatch),
        hadesmem::Error);

    // Check ScanData flag
    // Note: Pattern is for narrow string 'FindPattern' (without quotes)
    auto const find_pattern_str = find_pattern.Find(L"46 69 6E 64 50 61 74 "
        L"74 65 72 6E", hadesmem::FindPatternFlags::kScanData);
    BOOST_CHECK(find_pattern_str != nullptr);

    // Check conversion failures throw
    BOOST_CHECK_THROW(find_pattern.Find(L"ZZ",
        hadesmem::FindPatternFlags::kNone), hadesmem::Error);

    // Check conversion failures throw
    // Code changes now mean this causes an assert.
    //BOOST_CHECK_THROW(find_pattern.Find(L"", 
    //  hadesmem::FindPatternFlags::kNone), hadesmem::Error);
}

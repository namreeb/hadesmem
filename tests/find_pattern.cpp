// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/find_pattern.hpp>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>

// TODO: Clean up, expand, fix, etc these tests.

// TODO: Add more tests (e.g. stream overload tests).

void TestFindPattern()
{
  hadesmem::Process const process(::GetCurrentProcessId());

  HMODULE const self = GetModuleHandle(nullptr);
  BOOST_TEST_NE(self, static_cast<void*>(nullptr));

  hadesmem::FindPattern find_pattern(process);

  find_pattern = hadesmem::FindPattern(process);
  // Ensure constructor throws if an invalid module handle is specified
  // TODO: Fix this.
  // BOOST_CHECK_THROW(find_pattern = hadesmem::FindPattern(process,
  //  reinterpret_cast<HMODULE>(-1)), hadesmem::Error);

  // Scan for predicatable byte mask
  auto const nop =
    find_pattern.Find(L"", L"", L"90", hadesmem::FindPatternFlags::kNone, L"");
  BOOST_TEST_NE(nop, static_cast<void*>(nullptr));
  BOOST_TEST(nop > self);
  find_pattern.Find(L"", L"Nop", L"90", hadesmem::FindPatternFlags::kNone, L"");
  BOOST_TEST_EQ(nop, find_pattern.Lookup(L"", L"Nop"));
  auto const nop_rel = find_pattern.Find(
    L"", L"", L"90", hadesmem::FindPatternFlags::kRelativeAddress, L"");
  BOOST_TEST_EQ(static_cast<PBYTE>(nop_rel) + reinterpret_cast<DWORD_PTR>(self),
                nop);
  // TODO: Add checks for number of entries in the pattern map(s).

  auto const zeros = find_pattern.Find(
    L"", L"", L"00 ?? 00", hadesmem::FindPatternFlags::kNone, L"");
  BOOST_TEST_NE(zeros, static_cast<void*>(nullptr));
  BOOST_TEST(zeros > ::GetModuleHandleW(nullptr));
  find_pattern.Find(
    L"", L"Zeros", L"00 ?? 00", hadesmem::FindPatternFlags::kNone, L"");
  BOOST_TEST_EQ(zeros, find_pattern.Lookup(L"", L"Zeros"));
  BOOST_TEST_NE(nop, zeros);
  auto const zeros_rel = find_pattern.Find(
    L"", L"", L"00 ?? 00", hadesmem::FindPatternFlags::kRelativeAddress, L"");
  BOOST_TEST_EQ(
    static_cast<PBYTE>(zeros_rel) + reinterpret_cast<DWORD_PTR>(self), zeros);
  BOOST_TEST_EQ(find_pattern.PatternCount(L""), 2UL);

  // Todo: pattern_manipulators::Lea test

  std::wstring const pattern_file_data = LR"(
<?xml version="1.0" encoding="utf-8"?>
<HadesMem>
  <FindPattern>
    <Flag Name="RelativeAddress"/>
    <Flag Name="ThrowOnUnmatch"/>
    <Pattern Name="First Call" Data="E8">
      <Manipulator Name="Add" Operand1="1"/>
      <Manipulator Name="Rel" Operand1="5" Operand2="1"/>
    </Pattern>
    <Pattern Name="Zeros New" Data="00 ?? 00">
      <Manipulator Name="Add" Operand1="1"/>
      <Manipulator Name="Sub" Operand1="1"/>
    </Pattern>
    <Pattern Name="Nop Other" Data="90"/>
    <Pattern Name="Nop Second" Data="90" Start="Nop Other"/>
    <Pattern Name="FindPattern String" Data="46 69 6E 64 50 61 74 74 65 72 6E">
      <Flag Name="ScanData"/>
    </Pattern>
  </FindPattern>
</HadesMem>
)";
  // TODO: Test multiple modules properly.
  find_pattern.LoadPatternFileMemory(pattern_file_data);
  BOOST_TEST_EQ(find_pattern.ModuleCount(), 1UL);
  BOOST_TEST_EQ(find_pattern.PatternCount(L""), 7UL);
  BOOST_TEST_NE(find_pattern.Lookup(L"", L"First Call"),
                static_cast<void*>(nullptr));
  BOOST_TEST_EQ(find_pattern.Lookup(L"", L"Zeros New"), zeros_rel);
  BOOST_TEST_EQ(find_pattern.Lookup(L"", L"Nop Other"), nop_rel);
  BOOST_TEST(find_pattern.Lookup(L"", L"Nop Second") >
             find_pattern.Lookup(L"", L"Nop Other"));
  BOOST_TEST_NE(find_pattern.Lookup(L"", L"FindPattern String"),
                static_cast<void*>(nullptr));

  // TODO: Fix the test to ensure we get the error we're expecting, rather
  // than just any error.
  std::wstring const pattern_file_data_invalid1 = LR"(
<?xml version="1.0" encoding="utf-8"?>
<HadesMem>
  <FindPattern>
    <Flag Name="InvalidFlag"/>
  </FindPattern>
</HadesMem>
)";
  BOOST_TEST_THROWS(
    find_pattern.LoadPatternFileMemory(pattern_file_data_invalid1),
    hadesmem::Error);

  std::wstring const pattern_file_data_invalid2 = LR"(
<?xml version="1.0" encoding="utf-8"?>
<HadesMem>
  <FindPattern>
    <Flag Name="RelativeAddress"/>
    <Flag Name="ThrowOnUnmatch"/>
    <Pattern Name="Foo2" Data="ZZ"/>
  </FindPattern>
</HadesMem>
)";
  BOOST_TEST_THROWS(
    find_pattern.LoadPatternFileMemory(pattern_file_data_invalid2),
    hadesmem::Error);

  std::wstring const pattern_file_data_invalid3 = LR"(
<?xml version="1.0" encoding="utf-8"?>
<HadesMem>
  <FindPattern>
    <Flag Name="RelativeAddress"/>
    <Flag Name="ThrowOnUnmatch"/>
    <Pattern/>
  </FindPattern>
</HadesMem>
)";
  BOOST_TEST_THROWS(
    find_pattern.LoadPatternFileMemory(pattern_file_data_invalid3),
    hadesmem::Error);

  std::wstring const pattern_file_data_invalid4 = LR"(
<?xml version="1.0" encoding="utf-8"?>
<HadesMem>
  <FindPattern>
    <Flag Name="RelativeAddress"/>
    <Flag Name="ThrowOnUnmatch"/>
    <Pattern Name="Foo4" Data="90" Start="DoesNotExist"/>
  </FindPattern>
</HadesMem>
)";
  BOOST_TEST_THROWS(
    find_pattern.LoadPatternFileMemory(pattern_file_data_invalid4),
    hadesmem::Error);

  // Todo: LoadFile test

  auto const nops_any = find_pattern.Find(
    L"", L"", L"?? ?? ?? ?? ??", hadesmem::FindPatternFlags::kNone, L"");
  auto const int3s_any = find_pattern.Find(
    L"", L"", L"?? ?? ?? ?? ??", hadesmem::FindPatternFlags::kNone, L"");
  BOOST_TEST_EQ(nops_any, int3s_any);
  BOOST_TEST(nops_any > ::GetModuleHandle(nullptr));
  auto const nops_any_rel =
    find_pattern.Find(L"",
                      L"",
                      L"?? ?? ?? ?? ??",
                      hadesmem::FindPatternFlags::kRelativeAddress,
                      L"");
  BOOST_TEST_EQ(static_cast<PBYTE>(nops_any_rel) +
                  reinterpret_cast<DWORD_PTR>(self),
                nops_any);

  BOOST_TEST_THROWS(
    find_pattern.Find(
      L"",
      L"",
      L"AA BB CC DD EE FF 11 22 33 44 55 66 77 88 99 00 11 33 33 77",
      hadesmem::FindPatternFlags::kThrowOnUnmatch,
      L""),
    hadesmem::Error);

  // Pattern is for narrow string 'FindPattern' (without quotes)
  auto const find_pattern_str =
    find_pattern.Find(L"",
                      L"",
                      L"46 69 6E 64 50 61 74 74 65 72 6E",
                      hadesmem::FindPatternFlags::kScanData |
                        hadesmem::FindPatternFlags::kRelativeAddress,
                      L"");
  BOOST_TEST_NE(find_pattern_str, static_cast<void*>(nullptr));
  BOOST_TEST_EQ(find_pattern_str,
                find_pattern.Lookup(L"", L"FindPattern String"));

  auto const nop_second =
    find_pattern.Find(L"",
                      L"",
                      L"90",
                      hadesmem::FindPatternFlags::kRelativeAddress |
                        hadesmem::FindPatternFlags::kThrowOnUnmatch,
                      L"Nop Other");
  BOOST_TEST_NE(nop_second, static_cast<void*>(nullptr));
  BOOST_TEST_EQ(nop_second, find_pattern.Lookup(L"", L"Nop Second"));
  BOOST_TEST_THROWS(
    find_pattern.Find(L"",
                      L"",
                      L"90",
                      hadesmem::FindPatternFlags::kRelativeAddress |
                        hadesmem::FindPatternFlags::kThrowOnUnmatch |
                        hadesmem::FindPatternFlags::kScanData,
                      L"Nop Other"),
    hadesmem::Error);
  BOOST_TEST_NE(nop_second, static_cast<void*>(nullptr));
  BOOST_TEST_EQ(nop_second, find_pattern.Lookup(L"", L"Nop Second"));

  BOOST_TEST_THROWS(
    find_pattern.Find(L"", L"", L"ZZ", hadesmem::FindPatternFlags::kNone, L""),
    hadesmem::Error);
}

int main()
{
  TestFindPattern();
  return boost::report_errors();
}

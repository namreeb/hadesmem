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

// TODO: Add more tests.

void TestFindPattern()
{
  hadesmem::Process const process(::GetCurrentProcessId());

  std::uintptr_t const process_base =
    reinterpret_cast<std::uintptr_t>(::GetModuleHandleW(nullptr));

  void* nop =
    hadesmem::Find(process, L"", L"90", hadesmem::PatternFlags::kNone, 0U);
  BOOST_TEST_NE(nop, static_cast<void*>(nullptr));
  BOOST_TEST(nop > reinterpret_cast<void*>(process_base));

  void* nop_second =
    hadesmem::Find(process,
                   L"",
                   L"90",
                   hadesmem::PatternFlags::kNone,
                   reinterpret_cast<std::uintptr_t>(nop) - process_base);
  BOOST_TEST_NE(nop_second, static_cast<void*>(nullptr));
  BOOST_TEST_NE(nop_second, nop);
  BOOST_TEST(nop_second > nop);
  BOOST_TEST(nop_second > reinterpret_cast<void*>(process_base));

  void* find_pattern_string =
    hadesmem::Find(process,
                   L"",
                   L"46 ?? 6E 64 50 61 74 74 65 72 6E",
                   hadesmem::PatternFlags::kScanData,
                   0U);
  BOOST_TEST_NE(find_pattern_string, static_cast<void*>(nullptr));
  BOOST_TEST_NE(find_pattern_string, nop);
  BOOST_TEST(find_pattern_string > reinterpret_cast<void*>(process_base));

  BOOST_TEST_EQ(hadesmem::Find(process,
                               L"",
                               L"11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF",
                               hadesmem::PatternFlags::kNone,
                               0U),
                static_cast<void*>(nullptr));
  BOOST_TEST_THROWS(
    hadesmem::Find(process,
                   L"",
                   L"11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF",
                   hadesmem::PatternFlags::kThrowOnUnmatch,
                   0U),
    hadesmem::Error);

  void* rtl_random_string =
    hadesmem::Find(process,
                   L"ntdll.dll",
                   L"52 74 6c 52 61 6e ?? 6f 6d",
                   hadesmem::PatternFlags::kRelativeAddress,
                   0U);
  BOOST_TEST_NE(rtl_random_string, static_cast<void*>(nullptr));
  BOOST_TEST_NE(rtl_random_string, find_pattern_string);
  HMODULE const ntdll_mod = ::GetModuleHandleW(L"ntdll");
  std::uintptr_t const ntdll_base = reinterpret_cast<std::uintptr_t>(ntdll_mod);
  BOOST_TEST_NE(ntdll_mod, static_cast<HMODULE>(nullptr));
  std::string const rtlrandom = "RtlRandom";
  void* const rtl_random_string_absolute =
    static_cast<std::uint8_t*>(rtl_random_string) + ntdll_base;
  BOOST_TEST(std::equal(std::begin(rtlrandom),
                        std::end(rtlrandom),
                        static_cast<char const*>(rtl_random_string_absolute)));

  // Todo: Lea test
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
    <Pattern Name="FindPattern String" Data="46 ?? 6E 64 50 61 74 74 65 72 6E">
      <Flag Name="ScanData"/>
    </Pattern>
  </FindPattern>
  <FindPattern Module="ntdll.dll">
    <Flag Name="ThrowOnUnmatch"/>
    <Pattern Name="Int3 Then Nop" Data="CC 90"/>
    <Pattern Name="RtlRandom String" Data="52 74 6c 52 61 6e ?? 6f 6d"/>
  </FindPattern>
</HadesMem>
)";
  // TODO: Actually validate that the results are correct.
  hadesmem::FindPattern find_pattern(process, pattern_file_data, true);
  find_pattern = hadesmem::FindPattern(process, pattern_file_data, true);
  BOOST_TEST_EQ(find_pattern.GetModuleMap().size(), 2UL);
  BOOST_TEST_EQ(find_pattern.GetPatternMap(L"").size(), 5UL);
  BOOST_TEST_NE(find_pattern.Lookup(L"", L"First Call"),
                static_cast<void*>(nullptr));
  BOOST_TEST_NE(find_pattern.Lookup(L"", L"Zeros New"),
                static_cast<void*>(nullptr));
  BOOST_TEST_NE(find_pattern.Lookup(L"", L"Zeros New"),
                find_pattern.Lookup(L"", L"First Call"));
  BOOST_TEST_NE(find_pattern.Lookup(L"", L"Nop Other"),
                static_cast<void*>(nullptr));
  BOOST_TEST_EQ(
    find_pattern.Lookup(L"", L"Nop Other"),
    static_cast<void*>(static_cast<std::uint8_t*>(nop) - process_base));
  BOOST_TEST_NE(find_pattern.Lookup(L"", L"Nop Other"),
                find_pattern.Lookup(L"", L"First Call"));
  BOOST_TEST_NE(find_pattern.Lookup(L"", L"Nop Other"),
                find_pattern.Lookup(L"", L"Zeros New"));
  BOOST_TEST(find_pattern.Lookup(L"", L"Nop Second") >
             find_pattern.Lookup(L"", L"Nop Other"));
  BOOST_TEST_EQ(
    find_pattern.Lookup(L"", L"Nop Second"),
    static_cast<void*>(static_cast<std::uint8_t*>(nop_second) - process_base));
  BOOST_TEST_NE(find_pattern.Lookup(L"", L"FindPattern String"),
                static_cast<void*>(nullptr));
  BOOST_TEST_EQ(
    find_pattern.Lookup(L"", L"FindPattern String"),
    static_cast<void*>(static_cast<std::uint8_t*>(find_pattern_string) -
                       process_base));
  BOOST_TEST_EQ(find_pattern.GetPatternMap(L"ntdll.dll").size(), 2UL);
  BOOST_TEST_NE(find_pattern.Lookup(L"ntdll.dll", L"Int3 Then Nop"),
                static_cast<void*>(nullptr));
  auto const int3_then_nop =
    find_pattern.Lookup(L"ntdll.dll", L"Int3 Then Nop");
  BOOST_TEST_EQ(*static_cast<char const*>(int3_then_nop), '\xCC');
  BOOST_TEST_EQ(*(static_cast<char const*>(int3_then_nop) + 1), '\x90');
  BOOST_TEST_NE(find_pattern.Lookup(L"ntdll.dll", L"RtlRandom String"),
                static_cast<void*>(nullptr));
  BOOST_TEST(find_pattern.Lookup(L"ntdll.dll", L"RtlRandom String") >
             ntdll_mod);
  BOOST_TEST_EQ(find_pattern.Lookup(L"ntdll.dll", L"RtlRandom String"),
                rtl_random_string_absolute);
  BOOST_TEST_NE(find_pattern.Lookup(L"ntdll.dll", L"RtlRandom String"),
                find_pattern.Lookup(L"", L"FindPattern String"));
  BOOST_TEST_THROWS(find_pattern.Lookup(L"DoesNotExist", L"RtlRandom String"),
                    hadesmem::Error);
  BOOST_TEST_THROWS(find_pattern.Lookup(L"ntdll.dll", L"DoesNotExist"),
                    hadesmem::Error);

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
    hadesmem::FindPattern(process, pattern_file_data_invalid1, true),
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
    hadesmem::FindPattern(process, pattern_file_data_invalid2, true),
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
    hadesmem::FindPattern(process, pattern_file_data_invalid3, true),
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
    hadesmem::FindPattern(process, pattern_file_data_invalid4, true),
    hadesmem::Error);

  // Todo: LoadFile test
}

int main()
{
  TestFindPattern();
  return boost::report_errors();
}

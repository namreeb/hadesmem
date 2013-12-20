// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/module.hpp>
#include <hadesmem/module.hpp>

#include <utility>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/to_upper_ordinal.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/find_procedure.hpp>
#include <hadesmem/process.hpp>

// TODO: Test global var exports for FindProcedure.

void TestModule()
{
  hadesmem::Process const process(::GetCurrentProcessId());

  hadesmem::Module const this_mod(process, nullptr);
  BOOST_TEST_EQ(this_mod.GetHandle(), GetModuleHandle(nullptr));
  BOOST_TEST_NE(this_mod.GetSize(), 0U);
  BOOST_TEST(hadesmem::detail::ToUpperOrdinal(this_mod.GetName()) ==
             L"MODULE.EXE");
  BOOST_TEST(this_mod.GetPath().size() > this_mod.GetName().size());
  hadesmem::Module const this_mod_other(process, GetModuleHandle(nullptr));
  BOOST_TEST_EQ(this_mod, this_mod_other);
  BOOST_TEST(this_mod >= this_mod_other);
  BOOST_TEST(this_mod <= this_mod_other);
  BOOST_TEST(!(this_mod > this_mod_other));
  BOOST_TEST(!(this_mod < this_mod_other));
  BOOST_TEST_THROWS(FindProcedure(process, this_mod, "non_existant_export"),
                    hadesmem::Error);
  hadesmem::Module this_mod_copy(this_mod);
  BOOST_TEST_EQ(this_mod, this_mod_copy);
  hadesmem::Module this_mod_moved(std::move(this_mod_copy));
  BOOST_TEST_EQ(this_mod_moved, this_mod);
  this_mod_copy = std::move(this_mod_moved);
  BOOST_TEST_EQ(this_mod_copy, this_mod);

  BOOST_TEST_THROWS(hadesmem::Module(process, L""), hadesmem::Error);

  hadesmem::Module const ntdll_mod(process, L"NtDll.DlL");
  BOOST_TEST_NE(ntdll_mod, this_mod);
  BOOST_TEST_EQ(ntdll_mod.GetHandle(), ::GetModuleHandle(L"ntdll.dll"));
  BOOST_TEST_NE(ntdll_mod.GetSize(), 0U);
  BOOST_TEST(hadesmem::detail::ToUpperOrdinal(ntdll_mod.GetName()) ==
             L"NTDLL.DLL");
  BOOST_TEST(this_mod.GetPath().size() > this_mod.GetName().size());
  // Use an API that's unlikely to be hooked.
  BOOST_TEST_EQ(FindProcedure(process, ntdll_mod, "RtlRandom"),
                GetProcAddress(ntdll_mod.GetHandle(), "RtlRandom"));
  hadesmem::Module const ntdll_mod_other(process, L"ntdll.dll");
  BOOST_TEST_EQ(ntdll_mod, ntdll_mod_other);
  hadesmem::Module const ntdll_mod_from_handle(process,
                                               ::GetModuleHandle(L"ntdll.dll"));
  BOOST_TEST_EQ(ntdll_mod, ntdll_mod_from_handle);
  std::vector<wchar_t> system_path(HADESMEM_DETAIL_MAX_PATH_UNICODE);
  UINT const sys_path_len = GetSystemDirectory(
    system_path.data(), static_cast<UINT>(system_path.size()));
  BOOST_TEST_NE(sys_path_len, 0U);
  BOOST_TEST(sys_path_len <
             static_cast<UINT>(HADESMEM_DETAIL_MAX_PATH_UNICODE));
  std::wstring const ntdll_path =
    static_cast<std::wstring>(system_path.data()) + L"\\nTdLl.DlL";
  hadesmem::Module const ntdll_mod_from_path(process, ntdll_path);
  BOOST_TEST_EQ(ntdll_mod, ntdll_mod_from_path);

  if (GetModuleHandle(nullptr) < GetModuleHandle(L"ntdll.dll"))
  {
    BOOST_TEST(this_mod < ntdll_mod);
  }
  else
  {
    BOOST_TEST(this_mod > ntdll_mod);
  }

  std::stringstream test_str_1;
  test_str_1.imbue(std::locale::classic());
  test_str_1 << this_mod;
  std::stringstream test_str_2;
  test_str_2.imbue(std::locale::classic());
  test_str_2 << this_mod.GetHandle();
  BOOST_TEST_EQ(test_str_1.str(), test_str_2.str());
  std::stringstream test_str_3;
  test_str_3.imbue(std::locale::classic());
  test_str_3 << ntdll_mod;
  BOOST_TEST_NE(test_str_1.str(), test_str_3.str());
}

int main()
{
  TestModule();
  return boost::report_errors();
}

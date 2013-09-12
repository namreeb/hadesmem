// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/module.hpp>

#include <utility>

#define BOOST_TEST_MODULE module
#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/test/unit_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/find_procedure.hpp>
#include <hadesmem/detail/initialize.hpp>
#include <hadesmem/detail/to_upper_ordinal.hpp>

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

BOOST_TEST_DONT_PRINT_LOG_VALUE(std::wstring)
  
BOOST_AUTO_TEST_CASE(initialize)
{
  hadesmem::detail::InitializeAll();
}

BOOST_AUTO_TEST_CASE(module)
{
  hadesmem::Process const process(::GetCurrentProcessId());
  
  hadesmem::Module const this_mod(process, nullptr);
  BOOST_CHECK_EQUAL(this_mod.GetHandle(), GetModuleHandle(nullptr));
  BOOST_CHECK_NE(this_mod.GetSize(), 0U);
  BOOST_CHECK_EQUAL(hadesmem::detail::ToUpperOrdinal(this_mod.GetName()), 
    L"MODULE.EXE");
  BOOST_CHECK_GT(this_mod.GetPath().size(), this_mod.GetName().size());
  hadesmem::Module const this_mod_other(process, GetModuleHandle(nullptr));
  BOOST_CHECK_EQUAL(this_mod, this_mod_other);
  BOOST_CHECK_GE(this_mod, this_mod_other);
  BOOST_CHECK_LE(this_mod, this_mod_other);
  BOOST_CHECK(!(this_mod > this_mod_other));
  BOOST_CHECK(!(this_mod < this_mod_other));
  BOOST_CHECK_THROW(FindProcedure(process, this_mod, "non_existant_export"), 
    hadesmem::Error);
  hadesmem::Module this_mod_copy(this_mod);
  BOOST_CHECK_EQUAL(this_mod, this_mod_copy);
  hadesmem::Module this_mod_moved(std::move(this_mod_copy));
  BOOST_CHECK_EQUAL(this_mod_moved, this_mod);
  this_mod_copy = std::move(this_mod_moved);
  BOOST_CHECK_EQUAL(this_mod_copy, this_mod);
  
  hadesmem::Module const ntdll_mod(process, L"NtDll.DlL");
  BOOST_CHECK_NE(ntdll_mod, this_mod);
  BOOST_CHECK_EQUAL(ntdll_mod.GetHandle(), ::GetModuleHandle(L"ntdll.dll"));
  BOOST_CHECK_NE(ntdll_mod.GetSize(), 0U);
  BOOST_CHECK_EQUAL(hadesmem::detail::ToUpperOrdinal(ntdll_mod.GetName()), 
    L"NTDLL.DLL");
  BOOST_CHECK_GT(this_mod.GetPath().size(), this_mod.GetName().size());
  // Use an API that's unlikely to be hooked.
  BOOST_CHECK_EQUAL(
    FindProcedure(process, ntdll_mod, "RtlRandom"), 
    GetProcAddress(ntdll_mod.GetHandle(), "RtlRandom"));
  hadesmem::Module const ntdll_mod_other(process, L"ntdll.dll");
  BOOST_CHECK_EQUAL(ntdll_mod, ntdll_mod_other);
  hadesmem::Module const ntdll_mod_from_handle(process, 
    ::GetModuleHandle(L"ntdll.dll"));
  BOOST_CHECK_EQUAL(ntdll_mod, ntdll_mod_from_handle);
  std::vector<wchar_t> system_path(HADESMEM_DETAIL_MAX_PATH_UNICODE);
  UINT const sys_path_len = GetSystemDirectory(system_path.data(), 
    static_cast<UINT>(system_path.size()));
  BOOST_CHECK_NE(sys_path_len, 0U);
  BOOST_CHECK_LT(sys_path_len, static_cast<UINT>(HADESMEM_DETAIL_MAX_PATH_UNICODE));
  std::wstring const ntdll_path = static_cast<std::wstring>(
    system_path.data()) + L"\\nTdLl.DlL";
  hadesmem::Module const ntdll_mod_from_path(process, ntdll_path);
  BOOST_CHECK_EQUAL(ntdll_mod, ntdll_mod_from_path);
  
  if (GetModuleHandle(nullptr) < GetModuleHandle(L"ntdll.dll"))
  {
    BOOST_CHECK_LT(this_mod, ntdll_mod);
  }
  else
  {
    BOOST_CHECK_GT(this_mod, ntdll_mod);
  }

  std::stringstream test_str_1;
  test_str_1.imbue(std::locale::classic());
  test_str_1 << this_mod;
  std::stringstream test_str_2;
  test_str_2.imbue(std::locale::classic());
  test_str_2 << this_mod.GetHandle();
  BOOST_CHECK_EQUAL(test_str_1.str(), test_str_2.str());
  std::stringstream test_str_3;
  test_str_3.imbue(std::locale::classic());
  test_str_3 << ntdll_mod;
  BOOST_CHECK_NE(test_str_1.str(), test_str_3.str());
}

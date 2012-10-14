// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/module.hpp"

#define BOOST_TEST_MODULE module
#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/detail/to_upper_ordinal.hpp"

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

// TODO: Provide the appropriate stream operator overload to allow this (also 
// ensuring the streams used by Boost.Test are imbued with a UTF-8 locale).
BOOST_TEST_DONT_PRINT_LOG_VALUE(std::wstring)

// TODO: Test Module stream overloads.

BOOST_AUTO_TEST_CASE(module)
{
  hadesmem::Process const process(::GetCurrentProcessId());
  
  hadesmem::Module const this_mod(&process, nullptr);
  BOOST_CHECK_EQUAL(this_mod.GetHandle(), GetModuleHandle(nullptr));
  BOOST_CHECK_NE(this_mod.GetSize(), 0U);
  BOOST_CHECK_EQUAL(hadesmem::detail::ToUpperOrdinal(this_mod.GetName()), 
    L"MODULE.EXE");
  BOOST_CHECK_GT(this_mod.GetPath().size(), this_mod.GetName().size());
  hadesmem::Module const this_mod_other(&process, GetModuleHandle(nullptr));
  BOOST_CHECK_EQUAL(this_mod, this_mod_other);
  BOOST_CHECK_GE(this_mod, this_mod_other);
  BOOST_CHECK_LE(this_mod, this_mod_other);
  BOOST_CHECK(!(this_mod > this_mod_other));
  BOOST_CHECK(!(this_mod < this_mod_other));
  BOOST_CHECK_THROW(FindProcedure(this_mod, "non_existant_export"), 
    hadesmem::Error);
  hadesmem::Module this_mod_copy(this_mod);
  BOOST_CHECK_EQUAL(this_mod, this_mod_copy);
  hadesmem::Module this_mod_moved(std::move(this_mod_copy));
  BOOST_CHECK_EQUAL(this_mod_moved, this_mod);
  BOOST_CHECK(this_mod_moved != this_mod_copy);
  this_mod_copy = std::move(this_mod_moved);
  BOOST_CHECK_EQUAL(this_mod_copy, this_mod);
  BOOST_CHECK_NE(this_mod_copy, this_mod_moved);
  
  hadesmem::Module const ntdll_mod(&process, L"NtDll.DlL");
  BOOST_CHECK_NE(ntdll_mod, this_mod);
  BOOST_CHECK_EQUAL(ntdll_mod.GetHandle(), ::GetModuleHandle(L"ntdll.dll"));
  BOOST_CHECK_NE(ntdll_mod.GetSize(), 0U);
  BOOST_CHECK_EQUAL(hadesmem::detail::ToUpperOrdinal(ntdll_mod.GetName()), 
    L"NTDLL.DLL");
  BOOST_CHECK_GT(this_mod.GetPath().size(), this_mod.GetName().size());
  BOOST_CHECK_EQUAL(FindProcedure(ntdll_mod, "NtQueryInformationProcess"), 
    ::GetProcAddress(ntdll_mod.GetHandle(), "NtQueryInformationProcess"));
  hadesmem::Module const ntdll_mod_other(&process, L"ntdll.dll");
  BOOST_CHECK_EQUAL(ntdll_mod, ntdll_mod_other);
  hadesmem::Module const ntdll_mod_from_handle(&process, 
    ::GetModuleHandle(L"ntdll.dll"));
  BOOST_CHECK_EQUAL(ntdll_mod, ntdll_mod_from_handle);
  std::vector<wchar_t> system_path(MAX_PATH);
  UINT const sys_path_len = GetSystemDirectory(system_path.data(), MAX_PATH);
  BOOST_CHECK_NE(sys_path_len, 0U);
  BOOST_CHECK_LT(sys_path_len, static_cast<UINT>(MAX_PATH));
  std::wstring const ntdll_path = static_cast<std::wstring>(
    system_path.data()) + L"\\nTdLl.DlL";
  hadesmem::Module const ntdll_mod_from_path(&process, ntdll_path);
  BOOST_CHECK_EQUAL(ntdll_mod, ntdll_mod_from_path);
  
  if (GetModuleHandle(nullptr) < GetModuleHandle(L"ntdll.dll"))
  {
    BOOST_CHECK_LT(this_mod, ntdll_mod);
  }
  else
  {
    BOOST_CHECK_GT(this_mod, ntdll_mod);
  }
}

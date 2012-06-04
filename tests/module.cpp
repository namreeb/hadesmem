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
#include <boost/algorithm/string/case_conv.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"

// Boost.Test causes the following warning under GCC:
// error: base class 'struct boost::unit_test::ut_detail::nil_t' has a 
// non-virtual destructor [-Werror=effc++]
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic ignored "-Weffc++"
#endif // #if defined(HADESMEM_GCC)

BOOST_AUTO_TEST_CASE(module)
{
  hadesmem::Process const process(::GetCurrentProcessId());
  
  hadesmem::Module const this_mod(process, nullptr);
  BOOST_CHECK_EQUAL(this_mod.GetHandle(), GetModuleHandle(nullptr));
  BOOST_CHECK(this_mod.GetSize() != 0);
  BOOST_CHECK_EQUAL(boost::to_upper_copy(this_mod.GetName(), 
    std::locale::classic()), "MODULE.EXE");
  BOOST_CHECK(this_mod.GetPath().size() > this_mod.GetName().size());
  
  hadesmem::Module const ntdll_mod(process, "NtDll.DlL");
  BOOST_CHECK(ntdll_mod != this_mod);
  BOOST_CHECK_EQUAL(ntdll_mod.GetHandle(), ::GetModuleHandle(L"ntdll.dll"));
  BOOST_CHECK(ntdll_mod.GetSize() != 0);
  BOOST_CHECK_EQUAL(boost::to_upper_copy(ntdll_mod.GetName(), 
    std::locale::classic()), "NTDLL.DLL");
  BOOST_CHECK(this_mod.GetPath().size() > this_mod.GetName().size());
  BOOST_CHECK(ntdll_mod.FindProcedure("NtQueryInformationProcess") == 
    ::GetProcAddress(ntdll_mod.GetHandle(), "NtQueryInformationProcess"));
  hadesmem::Module const ntdll_mod_other(process, "ntdll.dll");
  BOOST_CHECK(ntdll_mod == ntdll_mod_other);
  hadesmem::Module const ntdll_mod_from_handle(process, 
    ::GetModuleHandle(L"ntdll.dll"));
  BOOST_CHECK(ntdll_mod == ntdll_mod_from_handle);
}

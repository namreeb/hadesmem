// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/injector.hpp"

#define BOOST_TEST_MODULE alloc
#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/test/unit_test.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/detail/self_path.hpp"

// Boost.Test causes the following warning under GCC:
// error: base class 'struct boost::unit_test::ut_detail::nil_t' has a 
// non-virtual destructor [-Werror=effc++]
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic ignored "-Weffc++"
#endif // #if defined(HADESMEM_GCC)

BOOST_AUTO_TEST_CASE(injector)
{
  hadesmem::Process const process(::GetCurrentProcessId());

  HMODULE const kernel32_mod = GetModuleHandle(L"kernel32.dll");
  BOOST_REQUIRE(kernel32_mod != nullptr);

  // 'Inject' Kernel32 with path resolution disabled and ensure that the 
  // module handle matches the one retrieved via GetModuleHandle earlier.
  HMODULE const kernel32_mod_new = hadesmem::InjectDll(process, 
    L"kernel32.dll", hadesmem::InjectFlags::kNone);
  BOOST_CHECK_EQUAL(kernel32_mod, kernel32_mod_new);

  // Call Kernel32.dll!FreeLibrary and ensure the return value and 
  // last error code are their expected values.
  std::pair<DWORD_PTR, DWORD> const export_ret = 
    CallExport(process, kernel32_mod_new, "FreeLibrary", kernel32_mod_new);
  BOOST_CHECK_NE(export_ret.first, static_cast<DWORD_PTR>(0));
  BOOST_CHECK_EQUAL(export_ret.second, 0UL);

  // Perform injection test again so we can test the FreeDll API.
  HMODULE const kernel32_mod_new_2 = hadesmem::InjectDll(process, 
    L"kernel32.dll", hadesmem::InjectFlags::kNone);
  BOOST_CHECK_EQUAL(kernel32_mod, kernel32_mod_new_2);

  // Free kernel32.dll in remote process.
  hadesmem::FreeDll(process, kernel32_mod_new_2);

  // Todo: Test path resolution.

  // TODO: Test export calling in CreateAndInject.
  // TODO: Test work dir, args, etc in CreateAndInject.
  hadesmem::CreateAndInjectData const inject_data = hadesmem::CreateAndInject(
    hadesmem::detail::GetSelfPath(), L"", std::vector<std::wstring>(), 
    L"d3d9.dll", "", nullptr, hadesmem::InjectFlags::kNone);
  BOOL const terminated = ::TerminateProcess(
    inject_data.GetProcess().GetHandle(), 0);
  DWORD const last_error = ::GetLastError();
  BOOST_CHECK_NE(terminated, 0);
  if (!terminated)
  {
    BOOST_CHECK_NE(last_error, 0UL);
  }
}

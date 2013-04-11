// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/injector.hpp>

#define BOOST_TEST_MODULE injector
#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/test/unit_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/call.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/detail/self_path.hpp>

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

  // Call Kernel32.dll!GetCurrentProcessId and ensure the return value and 
  // last error code are their expected values.
  SetLastError(0);
  auto const export_ret = 
    CallExport(process, kernel32_mod_new, "GetCurrentProcessId");
  BOOST_CHECK_EQUAL(export_ret.GetReturnValue(), GetCurrentProcessId());
  BOOST_CHECK_EQUAL(export_ret.GetLastError(), 0UL);

  // Perform injection test again so we can test the FreeDll API.
  HMODULE const kernel32_mod_new_2 = hadesmem::InjectDll(process, 
    L"kernel32.dll", hadesmem::InjectFlags::kNone);
  BOOST_CHECK_EQUAL(kernel32_mod, kernel32_mod_new_2);

  // Free kernel32.dll in remote process.
  hadesmem::FreeDll(process, kernel32_mod_new_2);

  // Todo: Test kPathResolution flag.

  // TODO: Test export calling in CreateAndInject.
  // TODO: Test work dir, args, etc in CreateAndInject.
  // TODO: Test kAddToSearchOrder flag.
  // TODO: Check more of the CreateAndInjectData functions.
  // TODO: Enumerate module list and ensure the target module has actually 
  // been loaded (will require a debug flag to not resume the target).
  // Check for presence of d3d9.dll to avoid accidentally creating a fork bomb.
  if (GetModuleHandle(L"d3d9.dll") == nullptr)
  {
    hadesmem::CreateAndInjectData const inject_data = 
      hadesmem::CreateAndInject(hadesmem::detail::GetSelfPath(), L"", 
      std::vector<std::wstring>(), L"d3d9.dll", "", 
      hadesmem::InjectFlags::kNone);
    BOOL const terminated = ::TerminateProcess(
      inject_data.GetProcess().GetHandle(), 0);
    DWORD const last_error = ::GetLastError();
    BOOST_CHECK_NE(terminated, 0);
    if (!terminated)
    {
      BOOST_CHECK_NE(last_error, 0UL);
    }
  }
}

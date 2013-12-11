// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/injector.hpp>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/call.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/detail/self_path.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>

void TestInjector()
{
  hadesmem::Process const process(::GetCurrentProcessId());

  HMODULE const kernel32_mod = GetModuleHandle(L"kernel32.dll");
  BOOST_TEST_NE(kernel32_mod, static_cast<HMODULE>(nullptr));

  // 'Inject' Kernel32 with path resolution disabled and ensure that the
  // module handle matches the one retrieved via GetModuleHandle earlier.
  HMODULE const kernel32_mod_new =
    hadesmem::InjectDll(process, L"kernel32.dll", hadesmem::InjectFlags::kNone);
  BOOST_TEST_EQ(kernel32_mod, kernel32_mod_new);

  // Call Kernel32.dll!GetCurrentProcessId and ensure the return value and
  // last error code are their expected values.
  SetLastError(0);
  auto const export_ret =
    CallExport(process, kernel32_mod_new, "GetCurrentProcessId");
  BOOST_TEST_EQ(export_ret.GetReturnValue(), GetCurrentProcessId());
  BOOST_TEST_EQ(export_ret.GetLastError(), 0UL);

  // Perform injection test again so we can test the FreeDll API.
  HMODULE const kernel32_mod_new_2 =
    hadesmem::InjectDll(process, L"kernel32.dll", hadesmem::InjectFlags::kNone);
  BOOST_TEST_EQ(kernel32_mod, kernel32_mod_new_2);

  // Free kernel32.dll in remote process.
  hadesmem::FreeDll(process, kernel32_mod_new_2);

  // Todo: Test kPathResolution flag.
  // TODO: Test kKeepSuspended flag.
  // TODO: Test export calling in CreateAndInject.
  // TODO: Test work dir, args, etc in CreateAndInject.
  // TODO: Test kAddToSearchOrder flag.
  // TODO: Check more of the CreateAndInjectData functions.
  // TODO: Enumerate module list and ensure the target module has actually
  // been loaded (will require a debug flag to not resume the target).
  // Check for presence of d3d9.dll to avoid accidentally creating a fork
  // bomb.
  std::vector<std::wstring> args;
  if (GetModuleHandle(L"d3d9.dll") == nullptr)
  {
    hadesmem::CreateAndInjectData const inject_data =
      hadesmem::CreateAndInject(hadesmem::detail::GetSelfPath(),
                                L"",
                                std::begin(args),
                                std::end(args),
                                L"d3d9.dll",
                                "",
                                hadesmem::InjectFlags::kNone);
    BOOL const terminated =
      ::TerminateProcess(inject_data.GetProcess().GetHandle(), 0);
    DWORD const last_error = ::GetLastError();
    BOOST_TEST_NE(terminated, 0);
    if (!terminated)
    {
      BOOST_TEST_NE(last_error, 0UL);
    }
  }
}

int main()
{
  TestInjector();
  return boost::report_errors();
}

// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/injector.hpp>
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
  // Don't cause a fork bomb when we later re-launch ourselves to test injection
  // as part of process creation.
  HMODULE const d3d9_mod = ::GetModuleHandleW(L"d3d9.dll");
  BOOST_TEST_EQ(d3d9_mod, static_cast<HMODULE>(nullptr));

  hadesmem::Process const process{::GetCurrentProcessId()};

  HMODULE const kernel32_mod = ::GetModuleHandleW(L"kernel32.dll");
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

  {
    std::vector<std::wstring> args;
    hadesmem::CreateAndInjectData const inject_data{
      hadesmem::CreateAndInject(hadesmem::detail::GetSelfPath(),
                                L"",
                                std::begin(args),
                                std::end(args),
                                L"d3d9.dll",
                                "",
                                hadesmem::InjectFlags::kNone)};
    BOOST_TEST(inject_data.GetProcess() != process);
    BOOST_TEST_NE(inject_data.GetModule(), static_cast<HMODULE>(nullptr));
    BOOST_TEST_EQ(inject_data.GetExportRet(), 0UL);
    BOOST_TEST_EQ(inject_data.GetExportLastError(), 0UL);
    BOOST_TEST_NE(inject_data.GetThreadHandle(), static_cast<HANDLE>(nullptr));
    BOOL const terminated =
      ::TerminateProcess(inject_data.GetProcess().GetHandle(), 0);
    BOOST_TEST_NE(terminated, 0);
  }

  {
    std::vector<std::wstring> args;
    hadesmem::CreateAndInjectData const inject_data{
      hadesmem::CreateAndInject(hadesmem::detail::GetSelfPath(),
                                L"",
                                std::begin(args),
                                std::end(args),
                                L"d3d9.dll",
                                "",
                                hadesmem::InjectFlags::kKeepSuspended)};
    BOOST_TEST(inject_data.GetProcess() != process);
    BOOST_TEST_NE(inject_data.GetModule(), static_cast<HMODULE>(nullptr));
    BOOST_TEST_EQ(inject_data.GetExportRet(), 0UL);
    BOOST_TEST_EQ(inject_data.GetExportLastError(), 0UL);
    BOOST_TEST_NE(inject_data.GetThreadHandle(), static_cast<HANDLE>(nullptr));
    inject_data.ResumeThread();
    BOOL const terminated =
      ::TerminateProcess(inject_data.GetProcess().GetHandle(), 0);
    BOOST_TEST_NE(terminated, 0);
  }
}

int main()
{
  TestInjector();
  return boost::report_errors();
}

// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/process.hpp>
#include <hadesmem/process.hpp>

#include <locale>
#include <sstream>
#include <utility>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/debug_privilege.hpp>
#include <hadesmem/detail/filesystem.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process_helpers.hpp>

void TestThisProcess()
{
  hadesmem::Process process(::GetCurrentProcessId());
  BOOST_TEST_EQ(process, process);
  hadesmem::Process process_new(::GetCurrentProcessId());
  BOOST_TEST_EQ(process, process_new);
  hadesmem::Process const process_moved(std::move(process_new));
  hadesmem::Process process_copy(process);
  BOOST_TEST_EQ(process_copy, process);
  process = process_copy;
  BOOST_TEST_EQ(process, process_copy);
  process_new = std::move(process_copy);
  BOOST_TEST_EQ(process, process_new);
  BOOST_TEST(process >= process);
  BOOST_TEST(process <= process);
  BOOST_TEST_EQ(process.GetId(), ::GetCurrentProcessId());
  std::stringstream process_str_1;
  process_str_1.imbue(std::locale::classic());
  process_str_1 << process;
  DWORD process_id_1;
  process_str_1 >> process_id_1;
  BOOST_TEST_EQ(process.GetId(), process_id_1);
  std::wstringstream process_str_2;
  process_str_2.imbue(std::locale::classic());
  process_str_2 << process;
  DWORD process_id_2;
  process_str_2 >> process_id_2;
  BOOST_TEST_EQ(process.GetId(), process_id_2);
  std::wstring const path(hadesmem::GetPath(process));
  BOOST_TEST(!path.empty());
  BOOST_TEST(hadesmem::detail::DoesFileExist(path));
  std::wstring const path_native(hadesmem::GetPathNative(process));
  BOOST_TEST(!path_native.empty());
  BOOL is_wow64_real = FALSE;
  BOOST_TEST(::IsWow64Process(GetCurrentProcess(), &is_wow64_real));
  BOOST_TEST_EQ(hadesmem::IsWoW64(process), is_wow64_real != FALSE);
  process.Cleanup();
  //BOOST_TEST_THROWS(hadesmem::GetSeDebugPrivilege(), hadesmem::Error);
}

int main()
{
  TestThisProcess();
  return boost::report_errors();
}

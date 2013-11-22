// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/process.hpp>

#include <locale>
#include <sstream>
#include <utility>

#define BOOST_TEST_MODULE process
#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/test/unit_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/debug_privilege.hpp>
#include <hadesmem/detail/filesystem.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process_helpers.hpp>

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

BOOST_AUTO_TEST_CASE(this_process)
{
  hadesmem::Process process(::GetCurrentProcessId());
  BOOST_CHECK_EQUAL(process, process);
  hadesmem::Process process_new(::GetCurrentProcessId());
  BOOST_CHECK_EQUAL(process, process_new);
  hadesmem::Process const process_moved(std::move(process_new));
  hadesmem::Process process_copy(process);
  BOOST_CHECK_EQUAL(process_copy, process);
  process = process_copy;
  BOOST_CHECK_EQUAL(process, process_copy);
  process_new = std::move(process_copy);
  BOOST_CHECK_EQUAL(process, process_new);
  BOOST_CHECK_GE(process, process);
  BOOST_CHECK_LE(process, process);
  BOOST_CHECK_EQUAL(process.GetId(), ::GetCurrentProcessId());
  std::stringstream process_str_1;
  process_str_1.imbue(std::locale::classic());
  process_str_1 << process;
  DWORD process_id_1;
  process_str_1 >> process_id_1;
  BOOST_CHECK_EQUAL(process.GetId(), process_id_1);
  std::wstringstream process_str_2;
  process_str_2.imbue(std::locale::classic());
  process_str_2 << process;
  DWORD process_id_2;
  process_str_2 >> process_id_2;
  BOOST_CHECK_EQUAL(process.GetId(), process_id_2);
  std::wstring const path(hadesmem::GetPath(process));
  BOOST_CHECK(!path.empty());
  BOOST_CHECK(hadesmem::detail::DoesFileExist(path));
  BOOL is_wow64_real = FALSE;
  BOOST_CHECK(::IsWow64Process(GetCurrentProcess(), &is_wow64_real));
  BOOST_CHECK_EQUAL(hadesmem::IsWoW64(process), is_wow64_real != FALSE);
  BOOST_CHECK_NO_THROW(process.Cleanup());
  BOOST_CHECK_THROW(hadesmem::GetSeDebugPrivilege(), hadesmem::Error);
}

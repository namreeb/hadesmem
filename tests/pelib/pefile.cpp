// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/pelib/pefile.hpp"

#include <sstream>
#include <utility>

#define BOOST_TEST_MODULE alloc
#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/test/unit_test.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/error.hpp"
#include "hadesmem/config.hpp"
#include "hadesmem/module.hpp"
#include "hadesmem/process.hpp"

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

BOOST_AUTO_TEST_CASE(pefile)
{
  hadesmem::Process const process(::GetCurrentProcessId());

  hadesmem::PeFile pe_file_1(process, GetModuleHandle(nullptr), 
    hadesmem::PeFileType::Image);

  hadesmem::PeFile pe_file_2(pe_file_1);
  BOOST_CHECK_EQUAL(pe_file_1, pe_file_2);
  pe_file_1 = pe_file_2;
  BOOST_CHECK_EQUAL(pe_file_1, pe_file_2);
  hadesmem::PeFile pe_file_3(std::move(pe_file_2));
  BOOST_CHECK_EQUAL(pe_file_3, pe_file_1);
  pe_file_2 = std::move(pe_file_3);
  BOOST_CHECK_EQUAL(pe_file_1, pe_file_2);

  hadesmem::PeFile pe_file_this(process, GetModuleHandle(nullptr), 
    hadesmem::PeFileType::Image);
  BOOST_CHECK_EQUAL(pe_file_this.GetBase(), GetModuleHandle(nullptr));
  BOOST_CHECK(pe_file_this.GetType() == hadesmem::PeFileType::Image);
  BOOST_CHECK_EQUAL(hadesmem::RvaToVa(process, pe_file_this, 0), 
    static_cast<void*>(nullptr));

  hadesmem::PeFile pe_file_ntdll(process, GetModuleHandle(L"ntdll"), 
    hadesmem::PeFileType::Image);
  BOOST_CHECK_EQUAL(pe_file_ntdll.GetBase(), GetModuleHandle(L"ntdll"));
  BOOST_CHECK(pe_file_ntdll.GetType() == hadesmem::PeFileType::Image);
  BOOST_CHECK_EQUAL(hadesmem::RvaToVa(process, pe_file_ntdll, 0), 
    static_cast<void*>(nullptr));

  BOOST_CHECK_EQUAL(pe_file_this, pe_file_this);
  BOOST_CHECK_NE(pe_file_this, pe_file_ntdll);
  BOOST_CHECK_NE(pe_file_ntdll, pe_file_this);
  if (pe_file_this > pe_file_ntdll)
  {
    BOOST_CHECK_GT(pe_file_this, pe_file_ntdll);
    BOOST_CHECK_GE(pe_file_this, pe_file_ntdll);
    BOOST_CHECK(!(pe_file_this < pe_file_ntdll));
    BOOST_CHECK(!(pe_file_this <= pe_file_ntdll));
  }
  else
  {
    BOOST_CHECK_GT(pe_file_ntdll, pe_file_this);
    BOOST_CHECK_GE(pe_file_ntdll, pe_file_this);
    BOOST_CHECK(!(pe_file_ntdll < pe_file_this));
    BOOST_CHECK(!(pe_file_ntdll <= pe_file_this));
  }

  std::stringstream test_str_1;
  test_str_1.imbue(std::locale::classic());
  test_str_1 << pe_file_this;
  std::stringstream test_str_2;
  test_str_2.imbue(std::locale::classic());
  test_str_2 << pe_file_this.GetBase();
  BOOST_CHECK_EQUAL(test_str_1.str(), test_str_2.str());
  std::stringstream test_str_3;
  test_str_3.imbue(std::locale::classic());
  test_str_3 << pe_file_ntdll.GetBase();
  BOOST_CHECK_NE(test_str_1.str(), test_str_3.str());
}

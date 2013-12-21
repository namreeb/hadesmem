// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/pelib/pe_file.hpp>

#include <sstream>
#include <utility>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/process.hpp>

void TestPeFile()
{
  hadesmem::Process const process(::GetCurrentProcessId());

  hadesmem::PeFile pe_file_1(
    process, ::GetModuleHandleW(nullptr), hadesmem::PeFileType::Image, 0);

  hadesmem::PeFile pe_file_2(pe_file_1);
  BOOST_TEST_EQ(pe_file_1, pe_file_2);
  pe_file_1 = pe_file_2;
  BOOST_TEST_EQ(pe_file_1, pe_file_2);
  hadesmem::PeFile pe_file_3(std::move(pe_file_2));
  BOOST_TEST_EQ(pe_file_3, pe_file_1);
  pe_file_2 = std::move(pe_file_3);
  BOOST_TEST_EQ(pe_file_1, pe_file_2);

  hadesmem::PeFile pe_file_this(
    process, ::GetModuleHandleW(nullptr), hadesmem::PeFileType::Image, 0);
  BOOST_TEST_EQ(pe_file_this.GetBase(), ::GetModuleHandle(nullptr));
  BOOST_TEST(pe_file_this.GetType() == hadesmem::PeFileType::Image);
  BOOST_TEST_EQ(hadesmem::RvaToVa(process, pe_file_this, 0),
                static_cast<void*>(nullptr));

  hadesmem::PeFile pe_file_ntdll(
    process, ::GetModuleHandleW(L"ntdll"), hadesmem::PeFileType::Image, 0);
  BOOST_TEST_EQ(pe_file_ntdll.GetBase(), ::GetModuleHandle(L"ntdll"));
  BOOST_TEST(pe_file_ntdll.GetType() == hadesmem::PeFileType::Image);
  BOOST_TEST_EQ(hadesmem::RvaToVa(process, pe_file_ntdll, 0),
                static_cast<void*>(nullptr));

  BOOST_TEST_EQ(pe_file_this, pe_file_this);
  BOOST_TEST_NE(pe_file_this, pe_file_ntdll);
  BOOST_TEST_NE(pe_file_ntdll, pe_file_this);
  if (pe_file_this > pe_file_ntdll)
  {
    BOOST_TEST(pe_file_this > pe_file_ntdll);
    BOOST_TEST(pe_file_this >= pe_file_ntdll);
    BOOST_TEST(!(pe_file_this < pe_file_ntdll));
    BOOST_TEST(!(pe_file_this <= pe_file_ntdll));
  }
  else
  {
    BOOST_TEST(pe_file_ntdll > pe_file_this);
    BOOST_TEST(pe_file_ntdll >= pe_file_this);
    BOOST_TEST(!(pe_file_ntdll < pe_file_this));
    BOOST_TEST(!(pe_file_ntdll <= pe_file_this));
  }

  std::stringstream test_str_1;
  test_str_1.imbue(std::locale::classic());
  test_str_1 << pe_file_this;
  std::stringstream test_str_2;
  test_str_2.imbue(std::locale::classic());
  test_str_2 << pe_file_this.GetBase();
  BOOST_TEST_EQ(test_str_1.str(), test_str_2.str());
  std::stringstream test_str_3;
  test_str_3.imbue(std::locale::classic());
  test_str_3 << pe_file_ntdll.GetBase();
  BOOST_TEST_NE(test_str_1.str(), test_str_3.str());
}

int main()
{
  TestPeFile();
  return boost::report_errors();
}

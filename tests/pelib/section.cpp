// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/pelib/section.hpp"

#include <sstream>
#include <utility>

#define BOOST_TEST_MODULE section
#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/test/unit_test.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/read.hpp"
#include "hadesmem/error.hpp"
#include "hadesmem/config.hpp"
#include "hadesmem/module.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/module_list.hpp"
#include "hadesmem/pelib/pe_file.hpp"
#include "hadesmem/pelib/nt_headers.hpp"

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

BOOST_AUTO_TEST_CASE(section)
{
  hadesmem::Process const process(::GetCurrentProcessId());

  hadesmem::PeFile pe_file_1(process, GetModuleHandle(nullptr), 
    hadesmem::PeFileType::Image);

  hadesmem::NtHeaders nt_headers_1(process, pe_file_1);

  BOOST_CHECK(nt_headers_1.GetNumberOfSections() >= 1);

  hadesmem::Section section_1(process, pe_file_1, 0);

  hadesmem::Section section_2(section_1);
  BOOST_CHECK_EQUAL(section_1, section_2);
  section_1 = section_2;
  BOOST_CHECK_EQUAL(section_1, section_2);
  hadesmem::Section section_3(std::move(section_2));
  BOOST_CHECK_EQUAL(section_3, section_1);
  section_2 = std::move(section_3);
  BOOST_CHECK_EQUAL(section_1, section_2);
  
  hadesmem::ModuleList modules(process);
  for (auto const& mod : modules)
  {
    // TODO: Also test FileType_Data
    hadesmem::PeFile const cur_pe_file(process, mod.GetHandle(), 
      hadesmem::PeFileType::Data);

    // Assume every module has at least one section.
    // TODO: Better tests.
    hadesmem::NtHeaders cur_nt_headers(process, cur_pe_file);
    BOOST_CHECK(cur_nt_headers.GetNumberOfSections() >= 1);
    hadesmem::Section cur_section(process, cur_pe_file, 0);
        
    auto const section_header_raw = hadesmem::Read<IMAGE_SECTION_HEADER>(
      process, cur_section.GetBase());

    cur_section.SetName(cur_section.GetName());
    cur_section.SetVirtualAddress(cur_section.GetVirtualAddress());
    cur_section.SetVirtualSize(cur_section.GetVirtualSize());
    cur_section.SetSizeOfRawData(cur_section.GetSizeOfRawData());
    cur_section.SetPointerToRawData(cur_section.GetPointerToRawData());
    cur_section.SetPointerToRelocations(cur_section.GetPointerToRelocations());
    cur_section.SetPointerToLinenumbers(cur_section.GetPointerToLinenumbers());
    cur_section.SetNumberOfRelocations(cur_section.GetNumberOfRelocations());
    cur_section.SetNumberOfLinenumbers(cur_section.GetNumberOfLinenumbers());
    cur_section.SetCharacteristics(cur_section.GetCharacteristics());
      
    auto const section_header_raw_new = hadesmem::Read<IMAGE_SECTION_HEADER>(
      process, cur_section.GetBase());
      
    BOOST_CHECK_EQUAL(std::memcmp(&section_header_raw, &section_header_raw_new, 
      sizeof(section_header_raw)), 0);

    std::stringstream test_str_1;
    test_str_1.imbue(std::locale::classic());
    test_str_1 << cur_section;
    std::stringstream test_str_2;
    test_str_2.imbue(std::locale::classic());
    test_str_2 << cur_section.GetBase();
    BOOST_CHECK_EQUAL(test_str_1.str(), test_str_2.str());
    if (mod.GetHandle() != GetModuleHandle(L"ntdll"))
    {
      hadesmem::PeFile const pe_file_ntdll(process, GetModuleHandle(L"ntdll"), 
        hadesmem::PeFileType::Image);
      hadesmem::Section const section_ntdll(process, pe_file_ntdll, 0);
      std::stringstream test_str_3;
      test_str_3.imbue(std::locale::classic());
      test_str_3 << section_ntdll.GetBase();
      BOOST_CHECK_NE(test_str_1.str(), test_str_3.str());
    }
  }
}

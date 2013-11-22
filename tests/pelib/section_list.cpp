// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/pelib/section_list.hpp>

#include <sstream>
#include <utility>

#define BOOST_TEST_MODULE section_list
#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/test/unit_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/read.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/module_list.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/pelib/section.hpp>
#include <hadesmem/pelib/nt_headers.hpp>

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

BOOST_AUTO_TEST_CASE(section_list)
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
    hadesmem::PeFile const pe_file(process, mod.GetHandle(), 
      hadesmem::PeFileType::Image);

    hadesmem::NtHeaders const nt_headers(process, pe_file);
    WORD const num_sections = nt_headers.GetNumberOfSections();

    // Assume every module has at least one section.
    // TODO: Better tests.
    hadesmem::SectionList sections(process, pe_file);
    WORD section_count = 0;
    for (auto& section : sections)
    {
      section_count = static_cast<WORD>(section_count + 1);

      auto const section_header_raw = hadesmem::Read<IMAGE_SECTION_HEADER>(
        process, section.GetBase());

      section.SetName(section.GetName());
      section.SetVirtualAddress(section.GetVirtualAddress());
      section.SetVirtualSize(section.GetVirtualSize());
      section.SetSizeOfRawData(section.GetSizeOfRawData());
      section.SetPointerToRawData(section.GetPointerToRawData());
      section.SetPointerToRelocations(section.GetPointerToRelocations());
      section.SetPointerToLinenumbers(section.GetPointerToLinenumbers());
      section.SetNumberOfRelocations(section.GetNumberOfRelocations());
      section.SetNumberOfLinenumbers(section.GetNumberOfLinenumbers());
      section.SetCharacteristics(section.GetCharacteristics());

      auto const section_header_raw_new = hadesmem::Read<IMAGE_SECTION_HEADER>(
        process, section.GetBase());

      BOOST_CHECK_EQUAL(std::memcmp(&section_header_raw, 
        &section_header_raw_new, sizeof(section_header_raw)), 0);

      std::stringstream test_str_1;
      test_str_1.imbue(std::locale::classic());
      test_str_1 << section;
      std::stringstream test_str_2;
      test_str_2.imbue(std::locale::classic());
      test_str_2 << section.GetBase();
      BOOST_CHECK_EQUAL(test_str_1.str(), test_str_2.str());
      if (mod.GetHandle() != GetModuleHandle(L"ntdll"))
      {
        hadesmem::PeFile const pe_file_ntdll(process, 
          GetModuleHandle(L"ntdll"), hadesmem::PeFileType::Image);
        hadesmem::Section const section_ntdll(process, pe_file_ntdll, 0);
        std::stringstream test_str_3;
        test_str_3.imbue(std::locale::classic());
        test_str_3 << section_ntdll.GetBase();
        BOOST_CHECK_NE(test_str_1.str(), test_str_3.str());
      }
    }
    BOOST_CHECK(section_count == num_sections);

    // Assume every module has a '.text' section.
    // TODO: Better tests.
    auto text_iter = std::find_if(std::begin(sections), std::end(sections), 
      [] (hadesmem::Section const& section)
      {
        return section.GetName() == ".data";
      });
    BOOST_CHECK(text_iter != std::end(sections));
  }
}

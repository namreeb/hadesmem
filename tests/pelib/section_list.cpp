// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/pelib/section_list.hpp>
#include <hadesmem/pelib/section_list.hpp>

#include <sstream>
#include <utility>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/module_list.hpp>
#include <hadesmem/pelib/nt_headers.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/pelib/section.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>

void TestSectionList()
{
  hadesmem::Process const process(::GetCurrentProcessId());

  hadesmem::PeFile pe_file_1(
    process, ::GetModuleHandleW(nullptr), hadesmem::PeFileType::Image, 0);

  hadesmem::NtHeaders nt_headers_1(process, pe_file_1);

  BOOST_TEST(nt_headers_1.GetNumberOfSections() >= 1);

  hadesmem::Section section_1(process, pe_file_1, 0);

  hadesmem::Section section_2(section_1);
  BOOST_TEST_EQ(section_1, section_2);
  section_1 = section_2;
  BOOST_TEST_EQ(section_1, section_2);
  hadesmem::Section section_3(std::move(section_2));
  BOOST_TEST_EQ(section_3, section_1);
  section_2 = std::move(section_3);
  BOOST_TEST_EQ(section_1, section_2);

  hadesmem::ModuleList modules(process);
  for (auto const& mod : modules)
  {
    // TODO: Also test PeFileType::Data
    hadesmem::PeFile const pe_file(
      process, mod.GetHandle(), hadesmem::PeFileType::Image, 0);

    hadesmem::NtHeaders const nt_headers(process, pe_file);
    WORD const num_sections = nt_headers.GetNumberOfSections();

    // Assume every module has at least one section.
    // TODO: Better tests.
    hadesmem::SectionList sections(process, pe_file);
    WORD section_count = 0;
    for (auto& section : sections)
    {
      section_count = static_cast<WORD>(section_count + 1);

      auto const section_header_raw =
        hadesmem::Read<IMAGE_SECTION_HEADER>(process, section.GetBase());

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
      section.UpdateWrite();
      section.UpdateRead();

      auto const section_header_raw_new =
        hadesmem::Read<IMAGE_SECTION_HEADER>(process, section.GetBase());

      BOOST_TEST_EQ(std::memcmp(&section_header_raw,
                                &section_header_raw_new,
                                sizeof(section_header_raw)),
                    0);

      std::stringstream test_str_1;
      test_str_1.imbue(std::locale::classic());
      test_str_1 << section;
      std::stringstream test_str_2;
      test_str_2.imbue(std::locale::classic());
      test_str_2 << section.GetBase();
      BOOST_TEST_EQ(test_str_1.str(), test_str_2.str());
      if (mod.GetHandle() != GetModuleHandle(L"ntdll"))
      {
        hadesmem::PeFile const pe_file_ntdll(process,
                                             ::GetModuleHandleW(L"ntdll"),
                                             hadesmem::PeFileType::Image,
                                             0);
        hadesmem::Section const section_ntdll(process, pe_file_ntdll, 0);
        std::stringstream test_str_3;
        test_str_3.imbue(std::locale::classic());
        test_str_3 << section_ntdll.GetBase();
        BOOST_TEST_NE(test_str_1.str(), test_str_3.str());
      }
    }
    BOOST_TEST(section_count == num_sections);

    // Assume every module has a '.text' section.
    // TODO: Better tests.
    auto text_iter = std::find_if(std::begin(sections),
                                  std::end(sections),
                                  [](hadesmem::Section const& section)
    { return section.GetName() == ".data"; });
    BOOST_TEST(text_iter != std::end(sections));
  }
}

int main()
{
  TestSectionList();
  return boost::report_errors();
}

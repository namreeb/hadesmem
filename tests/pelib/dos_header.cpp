// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/pelib/dos_header.hpp"

#include <sstream>
#include <utility>

#define BOOST_TEST_MODULE dos_header
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

BOOST_AUTO_TEST_CASE(dos_header)
{
  hadesmem::Process const process(::GetCurrentProcessId());

  hadesmem::PeFile pe_file_1(process, GetModuleHandle(nullptr), 
    hadesmem::PeFileType::Image);

  hadesmem::DosHeader dos_header_1(process, pe_file_1);

  hadesmem::DosHeader dos_header_2(dos_header_1);
  BOOST_CHECK_EQUAL(dos_header_1, dos_header_2);
  dos_header_1 = dos_header_2;
  BOOST_CHECK_EQUAL(dos_header_1, dos_header_2);
  hadesmem::DosHeader dos_header_3(std::move(dos_header_2));
  BOOST_CHECK_EQUAL(dos_header_3, dos_header_1);
  dos_header_2 = std::move(dos_header_3);
  BOOST_CHECK_EQUAL(dos_header_1, dos_header_2);
  
  hadesmem::ModuleList modules(process);
  for (auto const& mod : modules)
  {
    // TODO: Also test FileType_Data
    hadesmem::PeFile const pe_file(process, mod.GetHandle(), 
      hadesmem::PeFileType::Data);
      
    hadesmem::DosHeader dos_header(process, pe_file);
        
    auto const dos_header_raw = hadesmem::Read<IMAGE_DOS_HEADER>(process, 
      dos_header.GetBase());
      
    BOOST_CHECK_EQUAL(dos_header.IsValid(), true);
    dos_header.EnsureValid();
    dos_header.SetMagic(dos_header.GetMagic());
    dos_header.SetBytesOnLastPage(dos_header.GetBytesOnLastPage());
    dos_header.SetPagesInFile(dos_header.GetPagesInFile());
    dos_header.SetRelocations(dos_header.GetRelocations());
    dos_header.SetSizeOfHeaderInParagraphs(dos_header.
      GetSizeOfHeaderInParagraphs());
    dos_header.SetMinExtraParagraphs(dos_header.GetMinExtraParagraphs());
    dos_header.SetMaxExtraParagraphs(dos_header.GetMaxExtraParagraphs());
    dos_header.SetInitialSS(dos_header.GetInitialSS());
    dos_header.SetInitialSP(dos_header.GetInitialSP());
    dos_header.SetChecksum(dos_header.GetChecksum());
    dos_header.SetInitialIP(dos_header.GetInitialIP());
    dos_header.SetInitialCS(dos_header.GetInitialCS());
    dos_header.SetRelocTableFileAddr(dos_header.GetRelocTableFileAddr());
    dos_header.SetOverlayNum(dos_header.GetOverlayNum());
    dos_header.SetReservedWords1(dos_header.GetReservedWords1());
    dos_header.SetOEMID(dos_header.GetOEMID());
    dos_header.SetOEMInfo(dos_header.GetOEMInfo());
    dos_header.SetReservedWords2(dos_header.GetReservedWords2());
    dos_header.SetNewHeaderOffset(dos_header.GetNewHeaderOffset());
      
    auto const dos_header_raw_new = hadesmem::Read<IMAGE_DOS_HEADER>(process, 
      pe_file.GetBase());
      
    BOOST_CHECK_EQUAL(std::memcmp(&dos_header_raw, &dos_header_raw_new, 
      sizeof(dos_header_raw)), 0);

    std::stringstream test_str_1;
    test_str_1.imbue(std::locale::classic());
    test_str_1 << pe_file;
    std::stringstream test_str_2;
    test_str_2.imbue(std::locale::classic());
    test_str_2 << pe_file.GetBase();
    BOOST_CHECK_EQUAL(test_str_1.str(), test_str_2.str());
    if (mod.GetHandle() != GetModuleHandle(L"ntdll"))
    {
      hadesmem::PeFile const pe_file_ntdll(process, GetModuleHandle(L"ntdll"), 
        hadesmem::PeFileType::Image);
      std::stringstream test_str_3;
      test_str_3.imbue(std::locale::classic());
      test_str_3 << pe_file_ntdll.GetBase();
      BOOST_CHECK_NE(test_str_1.str(), test_str_3.str());
    }
  }
}

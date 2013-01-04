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
    hadesmem::PeFile const cur_pe_file(process, mod.GetHandle(), 
      hadesmem::PeFileType::Data);
      
    hadesmem::DosHeader cur_dos_header(process, cur_pe_file);
        
    auto const dos_header_raw = hadesmem::Read<IMAGE_DOS_HEADER>(process, 
      cur_dos_header.GetBase());
      
    BOOST_CHECK_EQUAL(cur_dos_header.IsValid(), true);
    cur_dos_header.EnsureValid();
    cur_dos_header.SetMagic(cur_dos_header.GetMagic());
    cur_dos_header.SetBytesOnLastPage(cur_dos_header.GetBytesOnLastPage());
    cur_dos_header.SetPagesInFile(cur_dos_header.GetPagesInFile());
    cur_dos_header.SetRelocations(cur_dos_header.GetRelocations());
    cur_dos_header.SetSizeOfHeaderInParagraphs(cur_dos_header.
      GetSizeOfHeaderInParagraphs());
    cur_dos_header.SetMinExtraParagraphs(
      cur_dos_header.GetMinExtraParagraphs());
    cur_dos_header.SetMaxExtraParagraphs(
      cur_dos_header.GetMaxExtraParagraphs());
    cur_dos_header.SetInitialSS(cur_dos_header.GetInitialSS());
    cur_dos_header.SetInitialSP(cur_dos_header.GetInitialSP());
    cur_dos_header.SetChecksum(cur_dos_header.GetChecksum());
    cur_dos_header.SetInitialIP(cur_dos_header.GetInitialIP());
    cur_dos_header.SetInitialCS(cur_dos_header.GetInitialCS());
    cur_dos_header.SetRelocTableFileAddr(
      cur_dos_header.GetRelocTableFileAddr());
    cur_dos_header.SetOverlayNum(cur_dos_header.GetOverlayNum());
    cur_dos_header.SetReservedWords1(cur_dos_header.GetReservedWords1());
    cur_dos_header.SetOEMID(cur_dos_header.GetOEMID());
    cur_dos_header.SetOEMInfo(cur_dos_header.GetOEMInfo());
    cur_dos_header.SetReservedWords2(cur_dos_header.GetReservedWords2());
    cur_dos_header.SetNewHeaderOffset(cur_dos_header.GetNewHeaderOffset());
      
    auto const dos_header_raw_new = hadesmem::Read<IMAGE_DOS_HEADER>(process, 
      cur_pe_file.GetBase());
      
    BOOST_CHECK_EQUAL(std::memcmp(&dos_header_raw, &dos_header_raw_new, 
      sizeof(dos_header_raw)), 0);

    std::stringstream test_str_1;
    test_str_1.imbue(std::locale::classic());
    test_str_1 << cur_dos_header;
    std::stringstream test_str_2;
    test_str_2.imbue(std::locale::classic());
    test_str_2 << cur_dos_header.GetBase();
    BOOST_CHECK_EQUAL(test_str_1.str(), test_str_2.str());
    if (mod.GetHandle() != GetModuleHandle(L"ntdll"))
    {
      hadesmem::PeFile const pe_file_ntdll(process, GetModuleHandle(L"ntdll"), 
        hadesmem::PeFileType::Image);
      hadesmem::DosHeader const dos_header_ntdll(process, pe_file_ntdll);
      std::stringstream test_str_3;
      test_str_3.imbue(std::locale::classic());
      test_str_3 << dos_header_ntdll.GetBase();
      BOOST_CHECK_NE(test_str_1.str(), test_str_3.str());
    }
  }
}

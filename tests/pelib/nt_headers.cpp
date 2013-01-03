// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/pelib/nt_headers.hpp"

#include <sstream>
#include <utility>

#define BOOST_TEST_MODULE nt_headers
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

BOOST_AUTO_TEST_CASE(nt_headers)
{
  hadesmem::Process const process(::GetCurrentProcessId());

  hadesmem::PeFile pe_file_1(process, GetModuleHandle(nullptr), 
    hadesmem::PeFileType::Image);

  hadesmem::NtHeaders nt_headers_1(process, pe_file_1);

  hadesmem::NtHeaders nt_headers_2(nt_headers_1);
  BOOST_CHECK_EQUAL(nt_headers_1, nt_headers_2);
  nt_headers_1 = nt_headers_2;
  BOOST_CHECK_EQUAL(nt_headers_1, nt_headers_2);
  hadesmem::NtHeaders nt_headers_3(std::move(nt_headers_2));
  BOOST_CHECK_EQUAL(nt_headers_3, nt_headers_1);
  nt_headers_2 = std::move(nt_headers_3);
  BOOST_CHECK_EQUAL(nt_headers_1, nt_headers_2);
  
  hadesmem::ModuleList modules(process);
  for (auto const& mod : modules)
  {
    // TODO: Also test FileType_Data
    hadesmem::PeFile const pe_file(process, mod.GetHandle(), 
      hadesmem::PeFileType::Data);
      
    hadesmem::NtHeaders nt_headers(process, pe_file);
        
    auto const nt_headers_raw = hadesmem::Read<IMAGE_NT_HEADERS>(process, 
      nt_headers.GetBase());
      
    BOOST_CHECK_EQUAL(nt_headers.IsValid(), true);
    nt_headers.EnsureValid();
    nt_headers.SetSignature(nt_headers.GetSignature());
    nt_headers.SetMachine(nt_headers.GetMachine());
    nt_headers.SetNumberOfSections(nt_headers.GetNumberOfSections());
    nt_headers.SetTimeDateStamp(nt_headers.GetTimeDateStamp());
    nt_headers.SetPointerToSymbolTable(nt_headers.
      GetPointerToSymbolTable());
    nt_headers.SetNumberOfSymbols(nt_headers.GetNumberOfSymbols());
    nt_headers.SetSizeOfOptionalHeader(nt_headers.
      GetSizeOfOptionalHeader());
    nt_headers.SetCharacteristics(nt_headers.GetCharacteristics());
    nt_headers.SetMagic(nt_headers.GetMagic());
    nt_headers.SetMajorLinkerVersion(nt_headers.GetMajorLinkerVersion());
    nt_headers.SetMinorLinkerVersion(nt_headers.GetMinorLinkerVersion());
    nt_headers.SetSizeOfCode(nt_headers.GetSizeOfCode());
    nt_headers.SetSizeOfInitializedData(nt_headers.
      GetSizeOfInitializedData());
    nt_headers.SetSizeOfUninitializedData(nt_headers.
      GetSizeOfUninitializedData());
    nt_headers.SetAddressOfEntryPoint(nt_headers.GetAddressOfEntryPoint());
    nt_headers.SetBaseOfCode(nt_headers.GetBaseOfCode());
#if defined(_M_IX86) 
    nt_headers.SetBaseOfData(nt_headers.GetBaseOfData());
#endif
    nt_headers.SetImageBase(nt_headers.GetImageBase());
    nt_headers.SetSectionAlignment(nt_headers.GetSectionAlignment());
    nt_headers.SetFileAlignment(nt_headers.GetFileAlignment());
    nt_headers.SetMajorOperatingSystemVersion(nt_headers.
      GetMajorOperatingSystemVersion());
    nt_headers.SetMinorOperatingSystemVersion(nt_headers.
      GetMinorOperatingSystemVersion());
    nt_headers.SetMajorImageVersion(nt_headers.GetMajorImageVersion());
    nt_headers.SetMinorImageVersion(nt_headers.GetMinorImageVersion());
    nt_headers.SetMajorSubsystemVersion(nt_headers.
      GetMajorSubsystemVersion());
    nt_headers.SetMinorSubsystemVersion(nt_headers.
      GetMinorSubsystemVersion());
    nt_headers.SetWin32VersionValue(nt_headers.GetWin32VersionValue());
    nt_headers.SetSizeOfImage(nt_headers.GetSizeOfImage());
    nt_headers.SetSizeOfHeaders(nt_headers.GetSizeOfHeaders());
    nt_headers.SetCheckSum(nt_headers.GetCheckSum());
    nt_headers.SetSubsystem(nt_headers.GetSubsystem());
    nt_headers.SetDllCharacteristics(nt_headers.GetDllCharacteristics());
    nt_headers.SetSizeOfStackReserve(nt_headers.GetSizeOfStackReserve());
    nt_headers.SetSizeOfStackCommit(nt_headers.GetSizeOfStackCommit());
    nt_headers.SetSizeOfHeapReserve(nt_headers.GetSizeOfHeapReserve());
    nt_headers.SetSizeOfHeapCommit(nt_headers.GetSizeOfHeapCommit());
    nt_headers.SetLoaderFlags(nt_headers.GetLoaderFlags());
    nt_headers.SetNumberOfRvaAndSizes(nt_headers.GetNumberOfRvaAndSizes());
    for (std::size_t i = 0; i < nt_headers.GetNumberOfRvaAndSizes(); ++i)
    {
      auto data_dir = static_cast<hadesmem::PeDataDir>(i);
      nt_headers.SetDataDirectoryVirtualAddress(data_dir, 
        nt_headers.GetDataDirectoryVirtualAddress(data_dir));
      nt_headers.SetDataDirectorySize(data_dir, 
        nt_headers.GetDataDirectorySize(data_dir));
    }
      
    auto const nt_headers_raw_new = hadesmem::Read<IMAGE_NT_HEADERS>(process, 
      nt_headers.GetBase());
      
    BOOST_CHECK_EQUAL(std::memcmp(&nt_headers_raw, &nt_headers_raw_new, 
      sizeof(nt_headers_raw)), 0);

    std::stringstream test_str_1;
    test_str_1.imbue(std::locale::classic());
    test_str_1 << nt_headers;
    std::stringstream test_str_2;
    test_str_2.imbue(std::locale::classic());
    test_str_2 << nt_headers.GetBase();
    BOOST_CHECK_EQUAL(test_str_1.str(), test_str_2.str());
    if (mod.GetHandle() != GetModuleHandle(L"ntdll"))
    {
      hadesmem::PeFile const pe_file_ntdll(process, GetModuleHandle(L"ntdll"), 
        hadesmem::PeFileType::Image);
      hadesmem::NtHeaders const nt_headers_ntdll(process, pe_file_ntdll);
      std::stringstream test_str_3;
      test_str_3.imbue(std::locale::classic());
      test_str_3 << nt_headers_ntdll.GetBase();
      BOOST_CHECK_NE(test_str_1.str(), test_str_3.str());
    }
  }
}

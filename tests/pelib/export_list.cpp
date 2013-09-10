// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/pelib/export_list.hpp>

#include <sstream>
#include <utility>

#define BOOST_TEST_MODULE export_list
#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/test/unit_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/read.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/module_list.hpp>
#include <hadesmem/pelib/export.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/pelib/export_dir.hpp>
#include <hadesmem/detail/initialize.hpp>

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

// Export something to ensure tests pass...
extern "C" HADESMEM_DETAIL_DLLEXPORT void Dummy();
extern "C" HADESMEM_DETAIL_DLLEXPORT void Dummy()
{ }

BOOST_AUTO_TEST_CASE(initialize)
{
  hadesmem::detail::InitializeAll();
}

BOOST_AUTO_TEST_CASE(export_list)
{
  hadesmem::Process const process(::GetCurrentProcessId());

  hadesmem::PeFile pe_file_1(process, GetModuleHandle(nullptr), 
    hadesmem::PeFileType::Image);

  bool processed_one_export_list = false;
  
  hadesmem::ModuleList modules(process);
  for (auto const& mod : modules)
  {
    // TODO: Also test FileType_Data
    hadesmem::PeFile const cur_pe_file(process, mod.GetHandle(), 
      hadesmem::PeFileType::Image);

    hadesmem::ExportList cur_export_list(process, cur_pe_file);
    if (std::begin(cur_export_list) == std::end(cur_export_list))
    {
      continue;
    }

    hadesmem::ExportDir cur_export_dir(process, cur_pe_file);

    processed_one_export_list = true;

    for (auto const& e : cur_export_list)
    {
      hadesmem::Export const test_export(process, cur_pe_file, e.GetOrdinal());

      // TODO: Ensure Export::ByName works
      if (test_export.ByName())
      {
        BOOST_CHECK(!test_export.GetName().empty());
      }
      else
      {
        BOOST_CHECK(test_export.ByOrdinal());
        BOOST_CHECK(test_export.GetOrdinal() >= 
          cur_export_dir.GetOrdinalBase());
      }

      // TODO: Ensure Export::Forwarded works
      if (test_export.IsForwarded())
      {
        BOOST_CHECK(!test_export.GetForwarder().empty());
        BOOST_CHECK(!test_export.GetForwarderModule().empty());
        BOOST_CHECK(!test_export.GetForwarderFunction().empty());
      }
      else
      {
        BOOST_CHECK(test_export.GetRva() != 0);
        BOOST_CHECK(test_export.GetVa() != nullptr);
      }

      std::stringstream test_str_1;
      test_str_1.imbue(std::locale::classic());
      test_str_1 << e;
      std::stringstream test_str_2;
      test_str_2.imbue(std::locale::classic());
      test_str_2 << e.GetOrdinal();
      BOOST_CHECK_EQUAL(test_str_1.str(), test_str_2.str());

      // TODO: Ensure that output differs across exports.
    }
  }

  BOOST_CHECK(processed_one_export_list);
}

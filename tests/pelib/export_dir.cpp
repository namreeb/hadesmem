// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/pelib/export_dir.hpp>
#include <hadesmem/pelib/export_dir.hpp>

#include <sstream>
#include <utility>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/module_list.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>

// Export something to ensure tests pass...
extern "C" HADESMEM_DETAIL_DLLEXPORT void Dummy();
extern "C" HADESMEM_DETAIL_DLLEXPORT void Dummy()
{
}

void TestExportDir()
{
  hadesmem::Process const process(::GetCurrentProcessId());

  hadesmem::PeFile pe_file_1(
    process, ::GetModuleHandleW(nullptr), hadesmem::PeFileType::Image, 0);

  hadesmem::ExportDir export_dir_1(process, pe_file_1);

  hadesmem::ExportDir export_dir_2(export_dir_1);
  BOOST_TEST_EQ(export_dir_1, export_dir_2);
  export_dir_1 = export_dir_2;
  BOOST_TEST_EQ(export_dir_1, export_dir_2);
  hadesmem::ExportDir export_dir_3(std::move(export_dir_2));
  BOOST_TEST_EQ(export_dir_3, export_dir_1);
  export_dir_2 = std::move(export_dir_3);
  BOOST_TEST_EQ(export_dir_1, export_dir_2);

  bool processed_one_export_dir = false;

  hadesmem::ModuleList modules(process);
  for (auto const& mod : modules)
  {
    // TODO: Also test PeFileType::Data
    hadesmem::PeFile const cur_pe_file(
      process, mod.GetHandle(), hadesmem::PeFileType::Image, 0);

    std::unique_ptr<hadesmem::ExportDir> cur_export_dir;
    try
    {
      cur_export_dir =
        std::make_unique<hadesmem::ExportDir>(process, cur_pe_file);
    }
    catch (std::exception const& /*e*/)
    {
      continue;
    }

    processed_one_export_dir = true;

    auto const export_dir_raw = hadesmem::Read<IMAGE_EXPORT_DIRECTORY>(
      process, cur_export_dir->GetBase());

    cur_export_dir->SetCharacteristics(cur_export_dir->GetCharacteristics());
    cur_export_dir->SetTimeDateStamp(cur_export_dir->GetTimeDateStamp());
    cur_export_dir->SetMajorVersion(cur_export_dir->GetMajorVersion());
    cur_export_dir->SetMinorVersion(cur_export_dir->GetMinorVersion());
    cur_export_dir->SetName(cur_export_dir->GetName());
    cur_export_dir->SetOrdinalBase(cur_export_dir->GetOrdinalBase());
    cur_export_dir->SetNumberOfFunctions(
      cur_export_dir->GetNumberOfFunctions());
    cur_export_dir->SetNumberOfNames(cur_export_dir->GetNumberOfNames());
    cur_export_dir->SetAddressOfFunctions(
      cur_export_dir->GetAddressOfFunctions());
    cur_export_dir->SetAddressOfNames(cur_export_dir->GetAddressOfNames());
    cur_export_dir->SetAddressOfNameOrdinals(
      cur_export_dir->GetAddressOfNameOrdinals());
    // Should the export dir name be the same as the module name under
    // normal circumstances?
    BOOST_TEST(!cur_export_dir->GetName().empty());

    auto const export_dir_raw_new = hadesmem::Read<IMAGE_EXPORT_DIRECTORY>(
      process, cur_export_dir->GetBase());

    BOOST_TEST_EQ(
      std::memcmp(&export_dir_raw, &export_dir_raw_new, sizeof(export_dir_raw)),
      0);

    std::stringstream test_str_1;
    test_str_1.imbue(std::locale::classic());
    test_str_1 << *cur_export_dir;
    std::stringstream test_str_2;
    test_str_2.imbue(std::locale::classic());
    test_str_2 << cur_export_dir->GetBase();
    BOOST_TEST_EQ(test_str_1.str(), test_str_2.str());

    // TODO: Ensure that base address is different across modules (similar to
    // other tests).
  }

  BOOST_TEST(processed_one_export_dir);
}

int main()
{
  TestExportDir();
  return boost::report_errors();
}

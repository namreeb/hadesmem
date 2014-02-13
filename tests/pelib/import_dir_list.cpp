// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/pelib/import_dir_list.hpp>
#include <hadesmem/pelib/import_dir_list.hpp>

#include <sstream>
#include <utility>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/module_list.hpp>
#include <hadesmem/pelib/import_dir.hpp>
#include <hadesmem/pelib/import_thunk.hpp>
#include <hadesmem/pelib/import_thunk_list.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>

// Export something to ensure tests pass...
extern "C" HADESMEM_DETAIL_DLLEXPORT void Dummy();
extern "C" HADESMEM_DETAIL_DLLEXPORT void Dummy()
{
}

void TestImportDirList()
{
  hadesmem::Process const process(::GetCurrentProcessId());

  hadesmem::PeFile pe_file_1(
    process, ::GetModuleHandleW(nullptr), hadesmem::PeFileType::Image, 0);

  bool processed_one_import_dir = false;

  hadesmem::ModuleList modules(process);
  for (auto const& mod : modules)
  {
    hadesmem::PeFile const cur_pe_file(
      process, mod.GetHandle(), hadesmem::PeFileType::Image, 0);

    hadesmem::ImportDirList import_dirs(process, cur_pe_file);
    if (mod.GetHandle() == GetModuleHandle(nullptr))
    {
      BOOST_TEST(std::begin(import_dirs) != std::end(import_dirs));

      auto iter = std::find_if(std::begin(import_dirs),
                               std::end(import_dirs),
                               [](hadesmem::ImportDir const& i)
      {
        return i.GetName() == "kernel32" || i.GetName() == "kernel32.dll" ||
               i.GetName() == "KERNEL32.dll" || i.GetName() == "KERNEL32.DLL";
      });
      BOOST_TEST(iter != std::end(import_dirs));

      hadesmem::ImportThunkList import_thunks(
        process, cur_pe_file, iter->GetOriginalFirstThunk());
      auto iter2 = std::find_if(std::begin(import_thunks),
                                std::end(import_thunks),
                                [](hadesmem::ImportThunk const& i)
      { return i.ByOrdinal() ? false : i.GetName() == "GetCurrentProcessId"; });
      BOOST_TEST(iter2 != std::end(import_thunks));
    }
    for (auto const& d : import_dirs)
    {
      hadesmem::ImportDir test_import_dir(
        process,
        cur_pe_file,
        reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(d.GetBase()));

      auto const imp_dir_raw = hadesmem::Read<IMAGE_IMPORT_DESCRIPTOR>(
        process, test_import_dir.GetBase());

      test_import_dir.SetOriginalFirstThunk(
        test_import_dir.GetOriginalFirstThunk());
      test_import_dir.SetTimeDateStamp(test_import_dir.GetTimeDateStamp());
      test_import_dir.SetForwarderChain(test_import_dir.GetForwarderChain());
      test_import_dir.SetNameRaw(test_import_dir.GetNameRaw());
      test_import_dir.SetName(test_import_dir.GetName());
      test_import_dir.SetFirstThunk(test_import_dir.GetFirstThunk());
      BOOST_TEST(!test_import_dir.GetName().empty());
      test_import_dir.UpdateWrite();
      test_import_dir.UpdateRead();

      auto const imp_dir_raw_new = hadesmem::Read<IMAGE_IMPORT_DESCRIPTOR>(
        process, test_import_dir.GetBase());

      BOOST_TEST_EQ(std::memcmp(&imp_dir_raw,
                                &imp_dir_raw_new,
                                sizeof(IMAGE_IMPORT_DESCRIPTOR)),
                    0);

      std::stringstream test_str_1;
      test_str_1.imbue(std::locale::classic());
      test_str_1 << test_import_dir;
      std::stringstream test_str_2;
      test_str_2.imbue(std::locale::classic());
      test_str_2 << test_import_dir.GetBase();
      BOOST_TEST_EQ(test_str_1.str(), test_str_2.str());

      hadesmem::ImportThunkList import_thunks(
        process, cur_pe_file, d.GetOriginalFirstThunk());
      BOOST_TEST(std::begin(import_thunks) != std::end(import_thunks));
      for (auto const& t : import_thunks)
      {
        hadesmem::ImportThunk test_thunk(
          process,
          cur_pe_file,
          reinterpret_cast<PIMAGE_THUNK_DATA>(t.GetBase()));

        auto const imp_thunk_raw =
          hadesmem::Read<IMAGE_THUNK_DATA>(process, test_thunk.GetBase());

        test_thunk.SetAddressOfData(test_thunk.GetAddressOfData());
        test_thunk.SetOrdinalRaw(test_thunk.GetOrdinalRaw());
        test_thunk.SetFunction(test_thunk.GetFunction());
        test_thunk.GetBase();
        if (test_thunk.ByOrdinal())
        {
          test_thunk.GetOrdinal();
        }
        else
        {
          test_thunk.GetHint();
          BOOST_TEST(!test_thunk.GetName().empty());
        }

        auto const imp_thunk_raw_new =
          hadesmem::Read<IMAGE_THUNK_DATA>(process, test_thunk.GetBase());

        BOOST_TEST_EQ(std::memcmp(&imp_thunk_raw,
                                  &imp_thunk_raw_new,
                                  sizeof(imp_thunk_raw)),
                      0);

        std::stringstream test_str_3;
        test_str_3.imbue(std::locale::classic());
        test_str_3 << test_thunk;
        std::stringstream test_str_4;
        test_str_4.imbue(std::locale::classic());
        test_str_4 << test_thunk.GetBase();
        BOOST_TEST_EQ(test_str_3.str(), test_str_4.str());

        processed_one_import_dir = true;
      }
    }
  }

  BOOST_TEST(processed_one_import_dir);
}

int main()
{
  TestImportDirList();
  return boost::report_errors();
}

// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "filesystem.hpp"

#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <vector>

#include <hadesmem/detail/filesystem.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/pelib/nt_headers.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>

#include "main.hpp"
#include "print.hpp"

void DumpFile(std::wstring const& path)
{
  try
  {
    std::wostream& out = GetOutputStreamW();

    SetCurrentFilePath(path);

    std::vector<char> buf;

    try
    {
      buf = hadesmem::detail::PeFileToBuffer(path);
    }
    catch (std::bad_alloc const&)
    {
      WriteNewline(out);
      WriteNormal(out, L"WARNING! File too large.", 0);
      WarnForCurrentFile(WarningType::kUnsupported);
      return;
    }
    catch (...)
    {
      return;
    }

    hadesmem::Process const process(GetCurrentProcessId());

    hadesmem::PeFile const pe_file(process,
                                   buf.data(),
                                   hadesmem::PeFileType::kData,
                                   static_cast<DWORD>(buf.size()));

    try
    {
      hadesmem::NtHeaders const nt_hdr(process, pe_file);
    }
    catch (std::exception const& /*e*/)
    {
      WriteNewline(out);
      WriteNormal(out, L"Not a PE file or wrong architecture (Pass 2).", 0);
      return;
    }

    DumpPeFile(process, pe_file, path);
  }
  catch (...)
  {
    std::cerr << "\nError!\n"
              << boost::current_exception_diagnostic_information() << '\n';

    auto const current_file_path = GetCurrentFilePath();
    if (!current_file_path.empty())
    {
      std::wcerr << "\nCurrent file: " << current_file_path << "\n";
    }
  }
}

void DumpDir(std::wstring const& path, hadesmem::detail::ThreadPool& pool)
{
  std::wostream& out = GetOutputStreamW();

  WriteNewline(out);
  WriteNormal(out, L"Entering dir: \"" + path + L"\".", 0);

  auto const f = [&](std::wstring const& cur_file) {
    std::wstring const cur_path = hadesmem::detail::MakeExtendedPath(
      hadesmem::detail::CombinePath(path, cur_file));

    WriteNewline(out);
    WriteNormal(out, L"Current path: \"" + cur_path + L"\".", 0);

    try
    {
      if (hadesmem::detail::IsDirectory(cur_path))
      {
        if (hadesmem::detail::IsSymlink(cur_path))
        {
          WriteNewline(out);
          WriteNormal(out, L"Skipping symlink.", 0);
        }
        else
        {
          DumpDir(cur_path, pool);
        }
      }
      else
      {
        auto const task = [cur_path]() { DumpFile(cur_path); };

        do
        {
          pool.WaitForSlot();
        } while (!pool.QueueTask(task));
      }
    }
    catch (hadesmem::Error const& e)
    {
      auto const last_error_ptr =
        boost::get_error_info<hadesmem::ErrorCodeWinLast>(e);
      if (last_error_ptr && *last_error_ptr == ERROR_SHARING_VIOLATION)
      {
        WriteNewline(out);
        WriteNormal(out, L"Sharing violation.", 0);
        return true;
      }

      if (last_error_ptr && *last_error_ptr == ERROR_ACCESS_DENIED)
      {
        WriteNewline(out);
        WriteNormal(out, L"Access denied.", 0);
        return true;
      }

      if (last_error_ptr && *last_error_ptr == ERROR_FILE_NOT_FOUND)
      {
        WriteNewline(out);
        WriteNormal(out, L"File not found.", 0);
        return true;
      }

      throw;
    }

    return true;
  };

  bool empty = false;
  bool access_denied = false;
  hadesmem::detail::EnumDir(path, f, &empty, &access_denied);

  if (empty)
  {
    WriteNewline(out);
    WriteNormal(out, L"Directory is empty.", 0);
    return;
  }

  if (access_denied)
  {
    WriteNewline(out);
    WriteNormal(out, L"Access denied to directory.", 0);
    return;
  }
}

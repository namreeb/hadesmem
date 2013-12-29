// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "filesystem.hpp"

#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <vector>

#include <hadesmem/detail/filesystem.hpp>
#include <hadesmem/detail/self_path.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/pelib/nt_headers.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>

#include "main.hpp"

namespace
{

// Modified code from http://bit.ly/1int3Iv.
// TODO: Use PathCchCanonicalizeEx on Windows 8+?
std::wstring MakeExtendedPath(std::wstring const& path)
{
  if (path.compare(0, 2, L"\\\\"))
  {
    // TODO: Fix this for paths longer than MAX_PATH. IsPathRelative and
    // CombinePath both use APIs which are limited to MAX_PATH.
    if (hadesmem::detail::IsPathRelative(path))
    {
      return MakeExtendedPath(hadesmem::detail::CombinePath(
        hadesmem::detail::GetSelfDirPath(), path));
    }
    else
    {
      return L"\\\\?\\" + path;
    }
  }
  else
  {
    if (path.compare(0, 3, L"\\\\?"))
    {
      return L"\\\\?\\UNC\\" + path.substr(2);
    }
    else
    {
      return path;
    }
  }
}
}

void DumpFile(std::wstring const& path)
{
  WarnForCurrentFile();

  std::unique_ptr<std::fstream> file_ptr(hadesmem::detail::OpenFileNarrow(
    path, std::ios::in | std::ios::binary | std::ios::ate));
  std::fstream& file = *file_ptr;
  if (!file)
  {
    std::wcout << "\nFailed to open file (" << path << ").\n";
    return;
  }

  std::streampos const size = file.tellg();
  if (size <= 0)
  {
    std::wcout << "\nEmpty or invalid file.\n";
    return;
  }

  if (!file.seekg(0, std::ios::beg))
  {
    std::wcout << "\nWARNING! Seeking to beginning of file failed (1).\n";
    return;
  }

  // Peek for the MZ header before reading the whole file.
  std::vector<char> mz_buf(2);
  if (!file.read(mz_buf.data(), 2))
  {
    std::wcout << "\nWARNING! Failed to read header signature.\n";
    return;
  }

  // Check for MZ signature
  if (mz_buf[0] != 'M' && mz_buf[1] != 'Z')
  {
    std::wcout << "\nNot a PE file (Pass 1).\n";
    return;
  }

  if (!file.seekg(0, std::ios::beg))
  {
    std::wcout << "\nWARNING! Seeking to beginning of file failed (2).\n";
    return;
  }

  std::vector<char> buf(static_cast<std::size_t>(size));

  if (!file.read(buf.data(), static_cast<std::streamsize>(size)))
  {
    std::wcout << "\nWARNING! Failed to read file data.\n";
    return;
  }

  hadesmem::Process const process(GetCurrentProcessId());

  hadesmem::PeFile const pe_file(process,
                                 buf.data(),
                                 hadesmem::PeFileType::Data,
                                 static_cast<DWORD>(buf.size()));

  try
  {
    hadesmem::NtHeaders const nt_hdr(process, pe_file);
  }
  catch (std::exception const& /*e*/)
  {
    std::wcout << "\nNot a PE file or wrong architecture (Pass 2).\n";
    return;
  }

  DumpPeFile(process, pe_file, path);
}

void DumpDir(std::wstring const& path)
{
  std::wcout << "\nEntering dir: \"" << path << "\".\n";

  std::wstring path_real(path);
  if (path_real.back() == L'\\')
  {
    path_real.pop_back();
  }

  WIN32_FIND_DATA find_data;
  ZeroMemory(&find_data, sizeof(find_data));
  hadesmem::detail::SmartFindHandle const handle(
    ::FindFirstFile((path_real + L"\\*").c_str(), &find_data));
  if (!handle.IsValid())
  {
    DWORD const last_error = ::GetLastError();
    if (last_error == ERROR_FILE_NOT_FOUND)
    {
      std::wcout << "\nDirectory is empty.\n";
      return;
    }
    if (last_error == ERROR_ACCESS_DENIED)
    {
      std::wcout << "\nAccess denied to directory.\n";
      return;
    }
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error() << hadesmem::ErrorString("FindFirstFile failed.")
                        << hadesmem::ErrorCodeWinLast(last_error));
  }

  do
  {
    std::wstring const cur_file = find_data.cFileName;
    if (cur_file == L"." || cur_file == L"..")
    {
      continue;
    }

    std::wstring const cur_path =
      MakeExtendedPath(path_real + L"\\" + cur_file);

    std::wcout << "\nCurrent path: \"" << cur_path << "\".\n";

    try
    {
      if (hadesmem::detail::IsDirectory(cur_path))
      {
        if (hadesmem::detail::IsSymlink(cur_path))
        {
          std::wcout << "\nSkipping symlink.\n";
        }
        else
        {
          DumpDir(cur_path);
        }
      }
      else
      {
        DumpFile(cur_path);
      }
    }
    catch (hadesmem::Error const& e)
    {
      auto const last_error_ptr =
        boost::get_error_info<hadesmem::ErrorCodeWinLast>(e);
      if (last_error_ptr && *last_error_ptr == ERROR_SHARING_VIOLATION)
      {
        std::wcout << "\nSharing violation.\n";
        continue;
      }

      if (last_error_ptr && *last_error_ptr == ERROR_ACCESS_DENIED)
      {
        std::wcout << "\nAccess denied.\n";
        continue;
      }

      if (last_error_ptr && *last_error_ptr == ERROR_FILE_NOT_FOUND)
      {
        std::wcout << "\nFile not found.\n";
        continue;
      }

      throw;
    }
  } while (::FindNextFile(handle.GetHandle(), &find_data));

  DWORD const last_error = ::GetLastError();
  if (last_error == ERROR_NO_MORE_FILES)
  {
    return;
  }
  HADESMEM_DETAIL_THROW_EXCEPTION(
    hadesmem::Error() << hadesmem::ErrorString("FindNextFile failed.")
                      << hadesmem::ErrorCodeWinLast(last_error));
}

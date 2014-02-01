// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <windows.h>
#include <shlwapi.h>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/detail/self_path.hpp>
#include <hadesmem/detail/smart_handle.hpp>
#include <hadesmem/detail/str_conv.hpp>

namespace hadesmem
{

namespace detail
{

// TODO: Add throwing variants of OpenFile*.

// libstdc++ doesn't support move operations on file-streams, so we have to
// return a unique_ptr instead. :(
inline std::unique_ptr<std::wfstream> OpenFileWide(std::wstring const& path,
                                                   std::ios_base::openmode mode)
{
#if defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
  return std::unique_ptr<std::wfstream>{new std::wfstream{path, mode}};
#else  // #if defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
  // libstdc++ doesn't support wide character overloads for ifstream's
  // construtor. :(
  return std::unique_ptr<std::wfstream>{
    new std::wfstream{hadesmem::detail::WideCharToMultiByte(path), mode}};
#endif // #if defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
}

// libstdc++ doesn't support move operations on file-streams, so we have to
// return a unique_ptr instead. :(
inline std::unique_ptr<std::fstream>
  OpenFileNarrow(std::wstring const& path, std::ios_base::openmode mode)
{
#if defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
  return std::unique_ptr<std::fstream>{new std::fstream{path, mode}};
#else  // #if defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
  // libstdc++ doesn't support wide character overloads for ifstream's
  // construtor. :(
  return std::unique_ptr<std::fstream>{
    new std::fstream{hadesmem::detail::WideCharToMultiByte(path), mode}};
#endif // #if defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
}

inline bool DoesFileExist(std::wstring const& path)
{
  return ::GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

inline bool IsPathRelative(std::wstring const& path)
{
  // TODO: Fix this for paths longer than MAX_PATH. (What other APIs are there?)
  return ::PathIsRelativeW(path.c_str()) != FALSE;
}

inline std::wstring CombinePath(std::wstring const& base,
                                std::wstring const& append)
{
  // TODO: Fix this for paths longer than MAX_PATH. (Use PathCchCombineEx for
  // Win 8+?)
  std::vector<wchar_t> buffer(MAX_PATH);
  if (!::PathCombineW(buffer.data(), base.c_str(), append.c_str()))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                    << ErrorString("PathCombineW failed.")
                                    << ErrorCodeWinLast(last_error));
  }

  return buffer.data();
}

inline SmartFileHandle OpenFileForMetadata(std::wstring const& path)
{
  HANDLE const file =
    ::CreateFileW(path.c_str(),
                  GENERIC_READ,
                  FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                  nullptr,
                  OPEN_EXISTING,
                  FILE_FLAG_BACKUP_SEMANTICS,
                  nullptr);
  if (file == INVALID_HANDLE_VALUE)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error() << ErrorString("CreateFile failed.")
                                            << ErrorCodeWinLast(last_error));
  }

  return SmartFileHandle(file);
}

inline BY_HANDLE_FILE_INFORMATION GetFileInformationByHandle(HANDLE file)
{
  BY_HANDLE_FILE_INFORMATION file_info;
  ::ZeroMemory(&file_info, sizeof(file_info));
  if (!::GetFileInformationByHandle(file, &file_info))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error() << ErrorString("GetFileInformationByHandle failed.")
              << ErrorCodeWinLast(last_error));
  }

  return file_info;
}

inline bool ArePathsEquivalent(std::wstring const& left,
                               std::wstring const& right)
{
  SmartFileHandle const left_file(OpenFileForMetadata(left));
  SmartFileHandle const right_file(OpenFileForMetadata(right));
  BY_HANDLE_FILE_INFORMATION left_file_info =
    GetFileInformationByHandle(left_file.GetHandle());
  BY_HANDLE_FILE_INFORMATION right_file_info =
    GetFileInformationByHandle(right_file.GetHandle());
  return left_file_info.dwVolumeSerialNumber ==
           right_file_info.dwVolumeSerialNumber &&
         left_file_info.nFileIndexHigh == right_file_info.nFileIndexHigh &&
         left_file_info.nFileIndexLow == right_file_info.nFileIndexLow &&
         left_file_info.nFileSizeHigh == right_file_info.nFileSizeHigh &&
         left_file_info.nFileSizeLow == right_file_info.nFileSizeLow &&
         left_file_info.ftLastWriteTime.dwLowDateTime ==
           right_file_info.ftLastWriteTime.dwLowDateTime &&
         left_file_info.ftLastWriteTime.dwHighDateTime ==
           right_file_info.ftLastWriteTime.dwHighDateTime;
}

inline std::wstring GetRootPath(std::wstring const& path)
{
  int const drive_num = ::PathGetDriveNumberW(path.c_str());
  if (drive_num == -1)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                    << ErrorString("PathGetDriveNumber failed.")
                                    << ErrorCodeWinLast(last_error));
  }

  std::vector<wchar_t> drive_path(4);
  ::PathBuildRoot(drive_path.data(), drive_num);
  if (drive_path[0] == L'\0' || drive_path[1] == L'\0' ||
      drive_path[2] == L'\0')
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                    << ErrorString("PathBuildRoot failed.")
                                    << ErrorCodeWinLast(last_error));
  }

  return drive_path.data();
}

inline DWORD GetFileAttributesWrapper(std::wstring const& path)
{
  DWORD const attributes = ::GetFileAttributesW(path.c_str());
  if (attributes == INVALID_FILE_ATTRIBUTES)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                    << ErrorString("GetFileAttributes failed.")
                                    << ErrorCodeWinLast(last_error));
  }

  return attributes;
}

inline bool IsDirectory(std::wstring const& path)
{
  DWORD const attributes =
    ::hadesmem::detail::GetFileAttributesWrapper(path.c_str());
  return !!(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

inline bool IsSymlink(std::wstring const& path)
{
  DWORD const attributes =
    ::hadesmem::detail::GetFileAttributesWrapper(path.c_str());
  return !!(attributes & FILE_ATTRIBUTE_REPARSE_POINT);
}

inline std::wstring GetFullPathNameWrapper(std::wstring const& path)
{
  std::vector<wchar_t> full_path(HADESMEM_DETAIL_MAX_PATH_UNICODE);
  DWORD len = GetFullPathNameW(path.c_str(),
                               static_cast<DWORD>(full_path.size()),
                               full_path.data(),
                               nullptr);
  if (!len || full_path.size() < len)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error() << ErrorString("GetFullPathNameW failed.")
              << ErrorCodeWinLast(last_error) << ErrorCodeWinOther(len));
  }

  return full_path.data();
}

// Modified code from http://bit.ly/1int3Iv.
// TODO: Use PathCchCanonicalizeEx because on Windows 8+?
inline std::wstring MakeExtendedPath(std::wstring const& path)
{
  if (path.compare(0, 2, L"\\\\"))
  {
    if (hadesmem::detail::IsPathRelative(path))
    {
      // ..\foo\bar
      return MakeExtendedPath(hadesmem::detail::CombinePath(
        hadesmem::detail::GetSelfDirPath(), path));
    }
    else
    {
      if (path.compare(0, 1, L"\\"))
      {
        // c:\foo\bar
        return L"\\\\?\\" + path;
      }
      else
      {
        // \foo\bar
        return MakeExtendedPath(hadesmem::detail::GetFullPathNameWrapper(path));
      }
    }
  }
  else
  {
    if (path.compare(0, 3, L"\\\\?"))
    {
      // \\server\share\folder
      return L"\\\\?\\UNC\\" + path.substr(2);
    }
    else
    {
      // \\?\c:\foo\bar
      return path;
    }
  }
}
}
}

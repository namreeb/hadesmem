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
  return ::GetFileAttributes(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

inline bool IsPathRelative(std::wstring const& path)
{
  return ::PathIsRelative(path.c_str()) != FALSE;
}

inline std::wstring CombinePath(std::wstring const& base,
                                std::wstring const& append)
{
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
    ::CreateFile(path.c_str(),
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
  int const drive_num = ::PathGetDriveNumber(path.c_str());
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

inline DWORD GetFileAttributes(std::wstring const& path)
{
  DWORD const attributes = ::GetFileAttributes(path.c_str());
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
  DWORD const attributes = ::hadesmem::detail::GetFileAttributes(path.c_str());
  return !!(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

inline bool IsSymlink(std::wstring const& path)
{
  DWORD const attributes = ::hadesmem::detail::GetFileAttributes(path.c_str());
  return !!(attributes & FILE_ATTRIBUTE_REPARSE_POINT);
}
}
}

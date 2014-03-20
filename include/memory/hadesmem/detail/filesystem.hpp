// Copyright (C) 2010-2014 Joshua Boyce.
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

#define HADESMEM_PATHCCH_ALLOW_LONG_PATHS 0x00000001

namespace hadesmem
{

namespace detail
{

// libstdc++ doesn't support move operations on file-streams, so we have to
// return a unique_ptr instead.
template <typename CharT>
inline std::unique_ptr<std::basic_fstream<CharT>>
  OpenFile(std::wstring const& path, std::ios_base::openmode mode)
{
#if defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
  return std::unique_ptr<std::basic_fstream<CharT>>{
    new std::basic_fstream<CharT>{path, mode}};
#else  // #if defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
  // libstdc++ doesn't support wide character overloads for ifstream's
  // construtor.
  return std::unique_ptr<std::basic_fstream<CharT>>{
    new std::basic_fstream<CharT>{hadesmem::detail::WideCharToMultiByte(path),
                                  mode}};
#endif // #if defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
}

inline bool DoesFileExist(std::wstring const& path)
{
  return ::GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

inline bool IsPathRelative(std::wstring const& path)
{
  // Relative paths are always limited to a total of MAX_PATH characters
  return (path.size() >= MAX_PATH) ? false : !!::PathIsRelativeW(path.c_str());
}

inline std::wstring CombinePath(std::wstring const& base,
                                std::wstring const& append)
{
  // Use newer and better PathCchCombineEx if it's available.
  detail::SmartModuleHandle const path_mod{
    LoadLibraryW(L"api-ms-win-core-path-l1-1-0.dll")};
  if (path_mod.IsValid())
  {
    using PathCchCombineExFn = HRESULT(WINAPI*)(PWSTR pszPathOut,
                                                size_t cchPathOut,
                                                PCWSTR pszPathIn,
                                                PCWSTR pszMore,
                                                unsigned long dwFlags);
    auto const path_cch_combine_ex = reinterpret_cast<PathCchCombineExFn>(
      GetProcAddress(path_mod.GetHandle(), "PathCchCombineEx"));
    if (path_cch_combine_ex)
    {
      std::vector<wchar_t> buffer(HADESMEM_DETAIL_MAX_PATH_UNICODE);
      HRESULT const hr = path_cch_combine_ex(buffer.data(),
                                             buffer.size(),
                                             base.c_str(),
                                             append.c_str(),
                                             HADESMEM_PATHCCH_ALLOW_LONG_PATHS);
      if (!SUCCEEDED(hr))
      {
        HADESMEM_DETAIL_THROW_EXCEPTION(
          Error{} << ErrorString{"PathCchCombineEx failed."}
                  << ErrorCodeWinHr{hr});
      }

      return buffer.data();
    }
  }

  // Fall back to older API with MAX_PATH limit.
  std::vector<wchar_t> buffer(MAX_PATH);
  if (!::PathCombineW(buffer.data(), base.c_str(), append.c_str()))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"PathCombineW failed."}
                                    << ErrorCodeWinLast{last_error});
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
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{} << ErrorString{"CreateFile failed."}
                                            << ErrorCodeWinLast{last_error});
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
      Error{} << ErrorString{"GetFileInformationByHandle failed."}
              << ErrorCodeWinLast{last_error});
  }

  return file_info;
}

inline bool ArePathsEquivalent(std::wstring const& left,
                               std::wstring const& right)
{
  SmartFileHandle const left_file{OpenFileForMetadata(left)};
  SmartFileHandle const right_file{OpenFileForMetadata(right)};
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
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"PathGetDriveNumber failed."}
                                    << ErrorCodeWinLast{last_error});
  }

  std::vector<wchar_t> drive_path(4);
  ::PathBuildRoot(drive_path.data(), drive_num);
  if (drive_path[0] == L'\0' || drive_path[1] == L'\0' ||
      drive_path[2] == L'\0')
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"PathBuildRoot failed."}
                                    << ErrorCodeWinLast{last_error});
  }

  return drive_path.data();
}

inline DWORD GetFileAttributesWrapper(std::wstring const& path)
{
  DWORD const attributes = ::GetFileAttributesW(path.c_str());
  if (attributes == INVALID_FILE_ATTRIBUTES)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"GetFileAttributes failed."}
                                    << ErrorCodeWinLast{last_error});
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
      Error{} << ErrorString{"GetFullPathNameW failed."}
              << ErrorCodeWinLast{last_error} << ErrorCodeWinOther{len});
  }

  return full_path.data();
}

inline std::wstring MakeExtendedPath(std::wstring path)
{
  // Use newer and better PathCchCanonicalizeEx if it's available, then fall
  // through to the custom implementation to ensure we always get an extended
  // path back out, rather than just when it's longer than MAX_PATH.
  detail::SmartModuleHandle const path_mod{
    LoadLibraryW(L"api-ms-win-core-path-l1-1-0.dll")};
  if (path_mod.IsValid())
  {
    using PathCchCanonicalizeExFn = HRESULT(WINAPI*)(PWSTR pszPathOut,
                                                     size_t cchPathOut,
                                                     PCWSTR pszPathIn,
                                                     unsigned long dwFlags);
    auto const path_cch_combine_ex = reinterpret_cast<PathCchCanonicalizeExFn>(
      GetProcAddress(path_mod.GetHandle(), "PathCchCanonicalizeEx"));
    if (path_cch_combine_ex)
    {
      std::vector<wchar_t> buffer(HADESMEM_DETAIL_MAX_PATH_UNICODE);
      HRESULT const hr = path_cch_combine_ex(buffer.data(),
                                             buffer.size(),
                                             path.c_str(),
                                             HADESMEM_PATHCCH_ALLOW_LONG_PATHS);
      if (!SUCCEEDED(hr))
      {
        HADESMEM_DETAIL_THROW_EXCEPTION(
          Error{} << ErrorString{"PathCchCanonicalizeEx failed."}
                  << ErrorCodeWinHr{hr});
      }

      path = buffer.data();
    }
  }

  // Modified code from http://bit.ly/1int3Iv.
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

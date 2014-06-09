// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include <windows.h>

#include <hadesmem/alloc.hpp>
#include <hadesmem/call.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/detail/argv_quote.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/filesystem.hpp>
#include <hadesmem/detail/force_initialize.hpp>
#include <hadesmem/detail/self_path.hpp>
#include <hadesmem/detail/static_assert.hpp>
#include <hadesmem/detail/smart_handle.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/find_procedure.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/write.hpp>

namespace hadesmem
{

struct InjectFlags
{
  enum : std::uint32_t
  {
    kNone = 0,
    kPathResolution = 1 << 0,
    kAddToSearchOrder = 1 << 1,
    kKeepSuspended = 1 << 2,
    kInvalidFlagMaxValue = 1 << 3
  };
};

inline HMODULE InjectDll(Process const& process,
                         std::wstring const& path,
                         std::uint32_t flags)
{
  HADESMEM_DETAIL_ASSERT(!(flags & ~(InjectFlags::kInvalidFlagMaxValue - 1UL)));

  bool const path_resolution = !!(flags & InjectFlags::kPathResolution);

  std::wstring const path_real = [&]() -> std::wstring
  {
    if (path_resolution && detail::IsPathRelative(path))
    {
      return detail::CombinePath(detail::GetSelfDirPath(), path);
    }

    return path;
  }();

  bool const add_path = !!(flags & InjectFlags::kAddToSearchOrder);
  if (add_path && detail::IsPathRelative(path_real))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error() << ErrorString("Cannot modify search order unless an absolute "
                             "path or path resolution is used."));
  }

  // Note: Only performing this check when path resolution is enabled,
  // because otherwise we would need to perform the check in the
  // context of the remote process, which is not possible to do without
  // introducing race conditions and other potential problems. So we
  // just let LoadLibraryExW do the check for us.
  if (path_resolution && !detail::DoesFileExist(path_real))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error() << ErrorString("Could not find module file."));
  }

  HADESMEM_DETAIL_TRACE_A("Calling ForceLdrInitializeThunk.");

  detail::ForceLdrInitializeThunk(process.GetId());

  HADESMEM_DETAIL_TRACE_FORMAT_W(L"Module path is \"%s\".", path_real.c_str());

  std::size_t const path_buf_size = (path_real.size() + 1) * sizeof(wchar_t);

  HADESMEM_DETAIL_TRACE_A("Allocating memory for module path.");

  Allocator const lib_file_remote{process, path_buf_size};

  HADESMEM_DETAIL_TRACE_A("Writing memory for module path.");

  WriteString(process, lib_file_remote.GetBase(), path_real);

  HADESMEM_DETAIL_TRACE_A("Finding LoadLibraryExW.");

  Module const kernel32_mod{process, L"kernel32.dll"};
  auto const load_library =
    FindProcedure(process, kernel32_mod, "LoadLibraryExW");

  HADESMEM_DETAIL_TRACE_A("Calling LoadLibraryExW.");

  auto const load_library_ret =
    Call(process,
         reinterpret_cast<decltype(&LoadLibraryExW)>(load_library),
         CallConv::kStdCall,
         static_cast<LPCWSTR>(lib_file_remote.GetBase()),
         nullptr,
         add_path ? LOAD_WITH_ALTERED_SEARCH_PATH : 0UL);
  if (!load_library_ret.GetReturnValue())
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error{} << ErrorString{"LoadLibraryExW failed."}
              << ErrorCodeWinLast{load_library_ret.GetLastError()});
  }

  return load_library_ret.GetReturnValue();
}

inline void FreeDll(Process const& process, HMODULE module)
{
  Module const kernel32_mod{process, L"kernel32.dll"};
  auto const free_library = FindProcedure(process, kernel32_mod, "FreeLibrary");

  auto const free_library_ret =
    Call(process,
         reinterpret_cast<decltype(&FreeLibrary)>(free_library),
         CallConv::kStdCall,
         module);
  if (!free_library_ret.GetReturnValue())
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error{} << ErrorString{"FreeLibrary failed."}
              << ErrorCodeWinLast{free_library_ret.GetLastError()});
  }
}

inline CallResult<DWORD_PTR> CallExport(Process const& process,
                                        HMODULE module,
                                        std::string const& export_name)
{
  Module const module_remote{process, module};
  auto const export_ptr = FindProcedure(process, module_remote, export_name);

  return Call(
    process, reinterpret_cast<DWORD_PTR (*)()>(export_ptr), CallConv::kDefault);
}

class CreateAndInjectData
{
public:
  explicit CreateAndInjectData(Process const& process,
                               HMODULE module,
                               DWORD_PTR export_ret,
                               DWORD export_last_error,
                               detail::SmartHandle&& thread_handle)
    : process_{process},
      module_{module},
      export_ret_{export_ret},
      export_last_error_{export_last_error},
      thread_handle_{std::move(thread_handle)}
  {
  }

  explicit CreateAndInjectData(Process&& process,
                               HMODULE module,
                               DWORD_PTR export_ret,
                               DWORD export_last_error,
                               detail::SmartHandle&& thread_handle) = delete;

#if defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

  CreateAndInjectData(CreateAndInjectData const&) = delete;

  CreateAndInjectData& operator=(CreateAndInjectData const&) = delete;

  CreateAndInjectData(CreateAndInjectData&& other)
    : process_{std::move(other.process_)},
      module_{other.module_},
      export_ret_{other.export_ret_},
      export_last_error_{other.export_last_error_},
      thread_handle_{std::move(other.thread_handle_)}
  {
  }

  CreateAndInjectData& operator=(CreateAndInjectData&& other)
  {
    process_ = std::move(other.process_);
    module_ = other.module_;
    export_ret_ = other.export_ret_;
    export_last_error_ = other.export_last_error_;
    thread_handle_ = std::move(other.thread_handle_);

    return *this;
  }

#else

  // Workaround for Intel C++ compile error (Intel C++ 2013 SP1 Updaate 1).

  CreateAndInjectData(CreateAndInjectData const&) = delete;

  CreateAndInjectData& operator=(CreateAndInjectData const&) = delete;

  CreateAndInjectData(CreateAndInjectData&&) = default;

  CreateAndInjectData& operator=(CreateAndInjectData&&) = default;

#endif // #if defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

  Process GetProcess() const
  {
    return process_;
  }

  HMODULE GetModule() const HADESMEM_DETAIL_NOEXCEPT
  {
    return module_;
  }

  DWORD_PTR GetExportRet() const HADESMEM_DETAIL_NOEXCEPT
  {
    return export_ret_;
  }

  DWORD GetExportLastError() const HADESMEM_DETAIL_NOEXCEPT
  {
    return export_last_error_;
  }

  HANDLE GetThreadHandle() const HADESMEM_DETAIL_NOEXCEPT
  {
    return thread_handle_.GetHandle();
  }

  void ResumeThread() const
  {
    if (::ResumeThread(thread_handle_.GetHandle()) == static_cast<DWORD>(-1))
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                      << ErrorString{"ResumeThread failed."}
                                      << ErrorCodeWinLast{last_error});
    }
  }

private:
  Process process_;
  HMODULE module_;
  DWORD_PTR export_ret_;
  DWORD export_last_error_;
  detail::SmartHandle thread_handle_;
};

template <typename ArgsIter>
inline CreateAndInjectData CreateAndInject(std::wstring const& path,
                                           std::wstring const& work_dir,
                                           ArgsIter args_beg,
                                           ArgsIter args_end,
                                           std::wstring const& module,
                                           std::string const& export_name,
                                           std::uint32_t flags)
{
  using ArgsIterValueType = typename std::iterator_traits<ArgsIter>::value_type;
  HADESMEM_DETAIL_STATIC_ASSERT(
    std::is_base_of<std::wstring, ArgsIterValueType>::value);

  std::wstring const command_line = [&]()
  {
    std::wstring command_line_temp;
    detail::ArgvQuote(&command_line_temp, path, false);
    auto const parse_arg = [&](std::wstring const& arg)
    {
      command_line_temp += L' ';
      detail::ArgvQuote(&command_line_temp, arg, false);
    };
    std::for_each(args_beg, args_end, parse_arg);
    return command_line_temp;
  }();

  std::vector<wchar_t> proc_args(std::begin(command_line),
                                 std::end(command_line));
  proc_args.push_back(L'\0');

  std::wstring const work_dir_real = [&]() -> std::wstring
  {
    if (work_dir.empty() && !path.empty() && !detail::IsPathRelative(path))
    {
      std::size_t const separator = path.find_last_of(L"\\/");
      if (separator != std::wstring::npos && separator != path.size() - 1)
      {
        return path.substr(0, separator + 1);
      }
    }

    return work_dir;
  }();

  STARTUPINFO start_info{};
  start_info.cb = static_cast<DWORD>(sizeof(start_info));
  PROCESS_INFORMATION proc_info{};
  if (!::CreateProcessW(path.c_str(),
                        proc_args.data(),
                        nullptr,
                        nullptr,
                        FALSE,
                        CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT,
                        nullptr,
                        work_dir_real.empty() ? nullptr : work_dir_real.c_str(),
                        &start_info,
                        &proc_info))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"CreateProcess failed."}
                                    << ErrorCodeWinLast{last_error});
  }

  detail::SmartHandle const proc_handle{proc_info.hProcess};
  detail::SmartHandle thread_handle{proc_info.hThread};

  try
  {
    Process const process{proc_info.dwProcessId};

    HMODULE const remote_module = InjectDll(process, module, flags);

    CallResult<DWORD_PTR> const export_ret = [&]()
    {
      if (!export_name.empty())
      {
        return CallExport(process, remote_module, export_name);
      }

      return CallResult<DWORD_PTR>(0, 0);
    }();

    if (!(flags & InjectFlags::kKeepSuspended))
    {
      if (::ResumeThread(thread_handle.GetHandle()) == static_cast<DWORD>(-1))
      {
        DWORD const last_error = ::GetLastError();
        HADESMEM_DETAIL_THROW_EXCEPTION(
          Error{} << ErrorString{"ResumeThread failed."}
                  << ErrorCodeWinLast{last_error}
                  << ErrorCodeWinRet{export_ret.GetReturnValue()}
                  << ErrorCodeWinOther{export_ret.GetLastError()});
      }
    }

    return CreateAndInjectData{process,
                               remote_module,
                               export_ret.GetReturnValue(),
                               export_ret.GetLastError(),
                               std::move(thread_handle)};
  }
  catch (std::exception const& /*e*/)
  {
    // Terminate process if injection failed, otherwise the 'zombie' process
    // would be leaked.
    BOOL const terminated = ::TerminateProcess(proc_handle.GetHandle(), 0);
    (void)terminated;
    HADESMEM_DETAIL_ASSERT(terminated != FALSE);

    throw;
  }
}
}

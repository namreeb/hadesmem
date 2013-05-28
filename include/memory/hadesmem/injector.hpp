// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <array>
#include <string>
#include <vector>
#include <utility>
#include <utility>
#include <iterator>
#include <algorithm>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/filesystem.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <windows.h>

#include <hadesmem/call.hpp>
#include <hadesmem/alloc.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/write.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/self_path.hpp>
#include <hadesmem/detail/smart_handle.hpp>

namespace hadesmem
{
  
namespace detail
{

inline void ArgvQuote(std::wstring* command_line, std::wstring const& argument, 
  bool force)
{
  // Unless we're told otherwise, don't quote unless we actually
  // need to do so (and hopefully avoid problems if programs won't
  // parse quotes properly).
  if (!force && !argument.empty() && argument.find_first_of(L" \t\n\v\"") 
    == argument.npos)
  {
    command_line->append(argument);
  }
  else 
  {
    command_line->push_back(L'"');

    for (auto it = std::begin(argument); ;++it)
    {
      std::size_t num_backslashes = 0;

      while (it != std::end(argument) && *it == L'\\') 
      {
        ++it;
        ++num_backslashes;
      }

      if (it == std::end(argument))
      {
        // Escape all backslashes, but let the terminating
        // double quotation mark we add below be interpreted
        // as a metacharacter.
        command_line->append(num_backslashes * 2, L'\\');
        break;
      }
      else if (*it == L'"')
      {
        // Escape all backslashes and the following
        // double quotation mark.
        command_line->append(num_backslashes * 2 + 1, L'\\');
        command_line->push_back(*it);
      }
      else
      {
        // Backslashes aren't special here.
        command_line->append(num_backslashes, L'\\');
        command_line->push_back(*it);
      }
    }

    command_line->push_back(L'"');
  }
}

inline void ForceLdrInitializeThunk(DWORD proc_id)
{
  Process const process(proc_id);

  // This is used to generate a 'nullsub' function, which is called in 
  // the context of the remote process in order to 'force' a call to 
  // ntdll.dll!LdrInitializeThunk. This is necessary because module 
  // enumeration will fail if LdrInitializeThunk has not been called, 
  // and Injector::InjectDll (and the APIs it uses) depends on the 
  // module enumeration APIs.
#if defined(HADESMEM_ARCH_X64) 
  std::array<BYTE, 1> return_instr = { { 0xC3 } };
#elif defined(HADESMEM_ARCH_X86) 
  std::array<BYTE, 3> return_instr = { { 0xC2, 0x04, 0x00 } };
#else 
#error "[HadesMem] Unsupported architecture."
#endif

  Allocator const stub_remote(process, sizeof(return_instr));

  Write(process, stub_remote.GetBase(), return_instr);

  auto const stub_remote_pfn = 
    reinterpret_cast<LPTHREAD_START_ROUTINE>(
    reinterpret_cast<DWORD_PTR>(stub_remote.GetBase()));

  detail::SmartHandle const remote_thread(::CreateRemoteThread(
    process.GetHandle(), nullptr, 0, stub_remote_pfn, nullptr, 0, nullptr));
  if (!remote_thread.GetHandle())
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Could not create remote thread.") << 
      ErrorCodeWinLast(last_error));
  }

  // TODO: Add a sensible and configurable timeout.
  if (::WaitForSingleObject(remote_thread.GetHandle(), INFINITE) != 
    WAIT_OBJECT_0)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Could not wait for remote thread.") << 
      ErrorCodeWinLast(last_error));
  }
}

}

template <typename T>
class CallResult;

struct InjectFlags
{
  enum
  {
    kNone = 0, 
    kPathResolution = 1 << 0, 
    kAddToSearchOrder = 1 << 1, 
    kKeepSuspended = 1 << 2, 
    kInvalidFlagMaxValue = 1 << 3
  };
};

inline HMODULE InjectDll(Process const& process, std::wstring const& path, 
  int flags)
{
  HADESMEM_ASSERT((flags & ~(InjectFlags::kInvalidFlagMaxValue - 1)) == 0);

   boost::filesystem::path path_real(path);

  bool const path_resolution = !!(flags & InjectFlags::kPathResolution);

  if (path_resolution && path_real.is_relative())
  {
    path_real = boost::filesystem::absolute(path_real, 
      detail::GetSelfDirPath());
  }
  path_real.make_preferred();

  bool const add_path = !!(flags & InjectFlags::kAddToSearchOrder);
  if (add_path && path_real.is_relative())
  {
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Cannot modify search order unless an absolute path "
      "or path resolution is used."));
  }

  // Note: Only performing this check when path resolution is enabled, 
  // because otherwise we would need to perform the check in the context 
  // of the remote process, which is not possible to do without 
  // introducing race conditions and other potential problems. So we just 
  // let LoadLibraryW do the check for us.
  if (path_resolution && !boost::filesystem::exists(path_real))
  {
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Could not find module file."));
  }

  detail::ForceLdrInitializeThunk(process.GetId());

  std::wstring const path_string(path_real.native());
  std::size_t const path_buf_size = (path_string.size() + 1) * 
    sizeof(wchar_t);

  Allocator const lib_file_remote(process, path_buf_size);
  WriteString(process, lib_file_remote.GetBase(), path_string);

  Module const kernel32_mod(process, L"kernel32.dll");
  auto const load_library = FindProcedure(kernel32_mod, "LoadLibraryExW");

  typedef HMODULE (LoadLibraryExFuncT)(LPCWSTR lpFileName, HANDLE hFile, 
    DWORD dwFlags);
  auto const load_library_ret = 
    Call<LoadLibraryExFuncT>(process, reinterpret_cast<FnPtr>(load_library), 
    CallConv::kWinApi, static_cast<LPCWSTR>(lib_file_remote.GetBase()), 
    nullptr, add_path ? LOAD_WITH_ALTERED_SEARCH_PATH : 0UL);
  if (!load_library_ret.GetReturnValue())
  {
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Call to LoadLibraryW in remote process failed.") << 
      ErrorCodeWinLast(load_library_ret.GetLastError()));
  }

  return load_library_ret.GetReturnValue();
}

inline void FreeDll(Process const& process, HMODULE module)
{
  Module const kernel32_mod(process, L"kernel32.dll");
  auto const free_library = FindProcedure(kernel32_mod, "FreeLibrary");

  typedef BOOL (FreeLibraryFuncT)(HMODULE hModule);
  auto const free_library_ret = 
    Call<FreeLibraryFuncT>(process, reinterpret_cast<FnPtr>(free_library), 
    CallConv::kWinApi, module);
  if (!free_library_ret.GetReturnValue())
  {
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Call to FreeLibrary in remote process failed.") << 
      ErrorCodeWinLast(free_library_ret.GetLastError()));
  }
}

// TODO: Configurable timeout.
inline CallResult<DWORD_PTR> CallExport(Process const& process, HMODULE module, 
  std::string const& export_name)
{
  Module const module_remote(process, module);
  auto const export_ptr = FindProcedure(module_remote, export_name);

  return Call<DWORD_PTR()>(process, reinterpret_cast<FnPtr>(export_ptr), 
    CallConv::kDefault);
}

class CreateAndInjectData
{
public:
  explicit CreateAndInjectData(Process const& process, HMODULE module, 
    DWORD_PTR export_ret, DWORD export_last_error)
    : process_(process), 
    module_(module), 
    export_ret_(export_ret), 
    export_last_error_(export_last_error)
  { }

  CreateAndInjectData(CreateAndInjectData const& other)
    : process_(other.process_), 
    module_(other.module_), 
    export_ret_(other.export_ret_), 
    export_last_error_(other.export_last_error_)
  { }

  CreateAndInjectData& operator=(CreateAndInjectData const& other)
  {
    process_ = other.process_;
    module_ = other.module_;
    export_ret_ = other.export_ret_;
    export_last_error_ = other.export_last_error_;

    return *this;
  }

  CreateAndInjectData(CreateAndInjectData&& other) HADESMEM_NOEXCEPT
    : process_(std::move(other.process_)), 
    module_(other.module_), 
    export_ret_(other.export_ret_), 
    export_last_error_(other.export_last_error_)
  { }

  CreateAndInjectData& operator=(CreateAndInjectData&& other) 
    HADESMEM_NOEXCEPT
  {
    process_ = std::move(other.process_);
    module_ = other.module_;
    export_ret_ = other.export_ret_;
    export_last_error_ = other.export_last_error_;

    return *this;
  }

  ~CreateAndInjectData() HADESMEM_NOEXCEPT
  { }

  Process GetProcess() const
  {
    return process_;
  }

  HMODULE GetModule() const HADESMEM_NOEXCEPT
  {
    return module_;
  }

  DWORD_PTR GetExportRet() const HADESMEM_NOEXCEPT
  {
    return export_ret_;
  }

  DWORD GetExportLastError() const HADESMEM_NOEXCEPT
  {
    return export_last_error_;
  }

private:
  Process process_;
  HMODULE module_;
  DWORD_PTR export_ret_;
  DWORD export_last_error_;
};

inline CreateAndInjectData CreateAndInject(
  std::wstring const& path, 
  std::wstring const& work_dir, 
  std::vector<std::wstring> const& args, 
  std::wstring const& module, 
  std::string const& export_name, 
  int flags)
{
  boost::filesystem::path const path_real(path);
  
  std::wstring command_line;
  detail::ArgvQuote(&command_line, path_real.native(), false);
  std::for_each(std::begin(args), std::end(args), 
    [&] (std::wstring const& arg) 
  {
    command_line += L' ';
    detail::ArgvQuote(&command_line, arg, false);
  });
  std::vector<wchar_t> proc_args(std::begin(command_line), 
    std::end(command_line));
  proc_args.push_back(L'\0');

  boost::filesystem::path work_dir_real;
  if (!work_dir.empty())
  {
    work_dir_real = work_dir;
  }
  else if (path_real.has_parent_path())
  {
    work_dir_real = path_real.parent_path();
  }
  else
  {
    work_dir_real = L"./";
  }

  STARTUPINFO start_info;
  ::ZeroMemory(&start_info, sizeof(start_info));
  start_info.cb = sizeof(start_info);
  PROCESS_INFORMATION proc_info;
  ::ZeroMemory(&proc_info, sizeof(proc_info));
  if (!::CreateProcess(path_real.c_str(), proc_args.data(), nullptr, nullptr, 
    FALSE, CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT, nullptr, 
    work_dir_real.c_str(), &start_info, &proc_info))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("CreateProcess failed.") << 
      ErrorCodeWinLast(last_error));
  }

  detail::SmartHandle const proc_handle(proc_info.hProcess);
  detail::SmartHandle const thread_handle(proc_info.hThread);

  try
  {
    Process const process(proc_info.dwProcessId);

    HMODULE const remote_module = InjectDll(process, module, flags);

    CallResult<DWORD_PTR> export_ret(0, 0);
    if (!export_name.empty())
    {
      // TODO: Configurable timeout.
      export_ret = CallExport(process, remote_module, export_name);
    }

    if (flags & InjectFlags::kKeepSuspended)
    {
      if (::ResumeThread(thread_handle.GetHandle()) == static_cast<DWORD>(-1))
      {
        DWORD const last_error = ::GetLastError();
        HADESMEM_THROW_EXCEPTION(Error() << 
          ErrorString("Could not resume process.") << 
          ErrorCodeWinLast(last_error) << 
          ErrorCodeWinRet(export_ret.GetReturnValue()) << 
          ErrorCodeWinOther(export_ret.GetLastError()));
      }
    }

    return CreateAndInjectData(process, remote_module, 
      export_ret.GetReturnValue(), export_ret.GetLastError());
  }
  catch (std::exception const& /*e*/)
  {
    // Terminate process if injection failed, otherwise the 'zombie' process 
    // would be leaked.
    ::TerminateProcess(proc_handle.GetHandle(), 0);

    throw;
  }
}

}

// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/injector.hpp"

#include <array>
#include <cassert>
#include <iterator>
#include <algorithm>

#include <windows.h>
#include <shlwapi.h>

#include "hadesmem/call.hpp"
#include "hadesmem/alloc.hpp"
#include "hadesmem/error.hpp"
#include "hadesmem/write.hpp"
#include "hadesmem/module.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/detail/self_path.hpp"
#include "hadesmem/detail/smart_handle.hpp"

// TODO: .NET injection (without DLL dependency if possible).

// TODO: Cross-session injection (also cross-winsta and cross-desktop 
// injection). RtlCreateUserThread can apparently inject across sessions but 
// creates a 'native' thread rather than a Win32 thread which causes issues. 
// A better solution is probably to use a broker process. Can the target's 
// WinSta and Desktop just be read from the PEB?

// TODO: IAT injection (to allow execution of code before Dllmain of other 
// modules are executed). Include support for .NET target processes.

// TODO: Get address of kernel32!LoadLibraryW 'manually' instead of using 
// local GetProcAddress and pointer arithmetic (whilst this works in all 
// normal cases, it will fail when the injector has shims enabled, and may 
// not work as expected when the injectee has shims enabled).

// TODO: Add flag to keep process paused after creation for debugging.

namespace hadesmem
{

namespace
{

void ArgvQuote(std::wstring* command_line, std::wstring const& argument, 
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

bool FileExists(std::wstring const& path)
{
  DWORD const attrib = ::GetFileAttributes(path.c_str());
  if (attrib == INVALID_FILE_ATTRIBUTES)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("GetFileAttributes failed.") << 
      ErrorCodeWinLast(last_error));
  }

  return !(attrib & FILE_ATTRIBUTE_DIRECTORY);
}

}

HMODULE InjectDll(Process const& process, std::wstring const& path, 
  int flags)
{
  assert((flags & ~(InjectFlags::kInvalidFlagMaxValue - 1)) == 0);

  // Do not continue if Shim Engine is enabled for local process, 
  // otherwise it could interfere with the address resolution.
  HMODULE const shim_eng_mod = ::GetModuleHandle(L"ShimEng.dll");
  if (shim_eng_mod)
  {
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Shims enabled for local process."));
  }

  std::wstring path_real(path);

  bool const path_resolution = !!(flags & InjectFlags::kPathResolution);

  if (path_resolution && PathIsRelative(path_real.c_str()))
  {
    std::array<wchar_t, MAX_PATH> absolute_path = { { 0 } };
    if (!PathCombine(absolute_path.data(), 
      detail::GetSelfDirPath().c_str(), 
      path_real.c_str()))
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_THROW_EXCEPTION(Error() << 
        ErrorString("PathCombine failed.") << 
        ErrorCodeWinLast(last_error));
    }
    path_real = absolute_path.data();
  }

  bool const add_path = !!(flags & InjectFlags::kAddToSearchOrder);
  if (add_path && PathIsRelative(path_real.c_str()))
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
  if (path_resolution && !FileExists(path_real))
  {
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Could not find module file."));
  }

  std::size_t const path_buf_size = (path_real.size() + 1) * 
    sizeof(wchar_t);

  Allocator const lib_file_remote(&process, path_buf_size);
  WriteString(process, lib_file_remote.GetBase(), path_real);

  Module const kernel32_mod(&process, L"kernel32.dll");
  auto const load_library = reinterpret_cast<LPCVOID>(
    reinterpret_cast<DWORD_PTR>(FindProcedure(kernel32_mod, 
    "LoadLibraryExW")));

  typedef HMODULE (*LoadLibraryExFuncT)(LPCWSTR lpFileName, HANDLE hFile, 
    DWORD dwFlags);
  auto const load_library_ret = 
    Call<LoadLibraryExFuncT>(process, load_library, CallConv::kWinApi, 
    static_cast<LPCWSTR>(lib_file_remote.GetBase()), nullptr, 
    add_path ? LOAD_WITH_ALTERED_SEARCH_PATH : 0UL);
  if (!load_library_ret.GetReturnValue())
  {
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Call to LoadLibraryW in remote process failed.") << 
      ErrorCodeWinLast(load_library_ret.GetLastError()));
  }

  return load_library_ret.GetReturnValue();
}

void FreeDll(Process const& process, HMODULE module)
{
  Module const kernel32_mod(&process, L"kernel32.dll");
  auto const free_library = reinterpret_cast<LPCVOID>(
    reinterpret_cast<DWORD_PTR>(FindProcedure(kernel32_mod, "FreeLibrary")));

  typedef BOOL (*FreeLibraryFuncT)(HMODULE hModule);
  auto const free_library_ret = 
    Call<FreeLibraryFuncT>(process, free_library, CallConv::kWinApi, module);
  if (!free_library_ret.GetReturnValue())
  {
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Call to FreeLibrary in remote process failed.") << 
      ErrorCodeWinLast(free_library_ret.GetLastError()));
  }
}

// TODO: Configurable timeout.
CallResult<DWORD_PTR> CallExport(Process const& process, HMODULE module, 
  std::string const& export_name)
{
  Module const module_remote(&process, module);
  auto const export_ptr = reinterpret_cast<LPCVOID>(
    reinterpret_cast<DWORD_PTR>(FindProcedure(module_remote, export_name)));

  return Call<DWORD_PTR(*)()>(process, export_ptr, CallConv::kDefault);
}

CreateAndInjectData::CreateAndInjectData(Process const& process, 
  HMODULE module, DWORD_PTR export_ret, DWORD export_last_error) 
  : process_(process), 
  module_(module), 
  export_ret_(export_ret), 
  export_last_error_(export_last_error)
{ }

CreateAndInjectData::CreateAndInjectData(CreateAndInjectData const& other)
  : process_(other.process_), 
  module_(other.module_), 
  export_ret_(other.export_ret_), 
  export_last_error_(other.export_last_error_)
{ }

CreateAndInjectData& CreateAndInjectData::operator=(
  CreateAndInjectData const& other)
{
  process_ = other.process_;
  module_ = other.module_;
  export_ret_ = other.export_ret_;
  export_last_error_ = other.export_last_error_;

  return *this;
}

CreateAndInjectData::CreateAndInjectData(CreateAndInjectData&& other) 
  HADESMEM_NOEXCEPT
  : process_(std::move(other.process_)), 
  module_(other.module_), 
  export_ret_(other.export_ret_), 
  export_last_error_(other.export_last_error_)
{
  other.module_ = nullptr;
  other.export_ret_ = 0;
  other.export_last_error_ = 0;
}

CreateAndInjectData& CreateAndInjectData::operator=(
  CreateAndInjectData&& other) HADESMEM_NOEXCEPT
{
  process_ = std::move(other.process_);

  module_ = other.module_;
  other.module_ = nullptr;

  export_ret_ = other.export_ret_;
  other.export_ret_ = 0;

  export_last_error_ = other.export_last_error_;
  other.export_last_error_ = 0;

  return *this;
}

Process CreateAndInjectData::GetProcess() const
{
  return process_;
}

HMODULE CreateAndInjectData::GetModule() const HADESMEM_NOEXCEPT
{
  return module_;
}

DWORD_PTR CreateAndInjectData::GetExportRet() const HADESMEM_NOEXCEPT
{
  return export_ret_;
}

DWORD CreateAndInjectData::GetExportLastError() const HADESMEM_NOEXCEPT
{
  return export_last_error_;
}

CreateAndInjectData CreateAndInject(
  std::wstring const& path, 
  std::wstring const& work_dir, 
  std::vector<std::wstring> const& args, 
  std::wstring const& module, 
  std::string const& export_name, 
  int flags)
{
  std::wstring command_line;
  ArgvQuote(&command_line, path, false);
  std::for_each(std::begin(args), std::end(args), 
    [&] (std::wstring const& arg) 
  {
    command_line += L' ';
    ArgvQuote(&command_line, arg, false);
  });
  std::vector<wchar_t> proc_args(std::begin(command_line), 
    std::end(command_line));
  proc_args.push_back(L'\0');

  std::wstring work_dir_real;
  if (!work_dir.empty())
  {
    work_dir_real = work_dir;
  }
  else if (path.rfind(L'\\') != std::string::npos)
  {
    work_dir_real = path.substr(0, path.rfind(L'\\') + 1);
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
  if (!::CreateProcess(path.c_str(), proc_args.data(), nullptr, nullptr, 
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

    // This is used to generate a 'nullsub' function, which is called in 
    // the context of the remote process in order to 'force' a call to 
    // ntdll.dll!LdrInitializeThunk. This is necessary because module 
    // enumeration will fail if LdrInitializeThunk has not been called, 
    // and Injector::InjectDll (and the APIs it uses) depend on the 
    // module enumeration APIs.
#if defined(_M_AMD64) 
    std::array<BYTE, 1> return_instr = { { 0xC3 } };
#elif defined(_M_IX86) 
    std::array<BYTE, 3> return_instr = { { 0xC2, 0x04, 0x00 } };
#else 
#error "[HadesMem] Unsupported architecture."
#endif

    Allocator const stub_remote(&process, sizeof(return_instr));

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

    // TODO: Add a sensible timeout.
    if (::WaitForSingleObject(remote_thread.GetHandle(), INFINITE) != 
      WAIT_OBJECT_0)
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_THROW_EXCEPTION(Error() << 
        ErrorString("Could not wait for remote thread.") << 
        ErrorCodeWinLast(last_error));
    }

    HMODULE const remote_module = InjectDll(process, module, flags);

    CallResult<DWORD_PTR> export_ret(0, 0);
    if (!export_name.empty())
    {
      // TODO: Configurable timeout.
      export_ret = CallExport(process, remote_module, export_name);
    }

    if (::ResumeThread(thread_handle.GetHandle()) == static_cast<DWORD>(-1))
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_THROW_EXCEPTION(Error() << 
        ErrorString("Could not resume process.") << 
        ErrorCodeWinLast(last_error) << 
        ErrorCodeWinRet(export_ret.GetReturnValue()) << 
        ErrorCodeWinOther(export_ret.GetLastError()));
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

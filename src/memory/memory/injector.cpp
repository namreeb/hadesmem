// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/injector.hpp"

#include <array>
#include <iterator>
#include <algorithm>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/filesystem.hpp>
#include <boost/scope_exit.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/call.hpp"
#include "hadesmem/alloc.hpp"
#include "hadesmem/error.hpp"
#include "hadesmem/write.hpp"
#include "hadesmem/module.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/detail/self_path.hpp"

// TODO: .NET injection (without DLL dependency if possible).
// TODO: Cross-session injection.
// TODO: IAT injection (to allow execution of code before Dllmain of other 
// modules are executed).
// TODO: Get address of kernel32!LoadLibraryW 'manually' instead of using 
// local GetProcAddress and pointer arithmetic (whilst this works in all 
// normal cases, it will fail when the injector has shims enabled, and may 
// not work as expected when the injectee has shims enabled).

namespace hadesmem
{

namespace
{

void ArgvQuote(std::wstring* command_line, std::wstring const& argument, 
  bool force)
{
  // Unless we're told otherwise, don't quote unless we actually
  // need to do so --- hopefully avoid problems if programs won't
  // parse quotes properly
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

}

HMODULE InjectDll(Process const& process, std::wstring const& path, 
  int flags)
{
  BOOST_ASSERT((flags & ~(InjectFlags::kInvalidFlagMaxValue - 1)) == 0);

  // Do not continue if Shim Engine is enabled for local process, 
  // otherwise it could interfere with the address resolution.
  HMODULE const shim_eng_mod = GetModuleHandle(L"ShimEng.dll");
  if (shim_eng_mod)
  {
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Shims enabled for local process."));
  }

  boost::filesystem::path path_real(path);

  bool const path_resolution = !!(flags & InjectFlags::kPathResolution);

  if (path_resolution && path_real.is_relative())
  {
    path_real = boost::filesystem::absolute(path_real, 
      detail::GetSelfDirPath());
  }

  path_real.make_preferred();

  // Ensure target file exists
  // Note: Only performing this check when path resolution is enabled, 
  // because otherwise we would need to perform the check in the context 
  // of the remote process, which is not possible to do without 
  // introducing race conditions and other potential problems. So we just 
  // let LoadLibraryW do the check for us.
  if (path_resolution && !boost::filesystem::exists(path_real))
  {
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Could not find module file."));
  }

  std::wstring const path_string(path_real.native());
  std::size_t const path_buf_size = (path_string.size() + 1) * 
    sizeof(wchar_t);

  Allocator const lib_file_remote(&process, path_buf_size);
  WriteString(process, lib_file_remote.GetBase(), path_string);

  Module const kernel32_mod(&process, L"kernel32.dll");
  LPCVOID const load_library = reinterpret_cast<LPCVOID>(
    reinterpret_cast<DWORD_PTR>(FindProcedure(kernel32_mod, "LoadLibraryW")));

  typedef HMODULE (*LoadLibraryFuncT)(LPCWSTR lpFileName);
  std::pair<HMODULE, DWORD> const load_library_ret = 
    Call<LoadLibraryFuncT>(process, load_library, CallConv::kWinApi, 
    static_cast<LPCWSTR>(lib_file_remote.GetBase()));
  if (!load_library_ret.first)
  {
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Call to LoadLibraryW in remote process failed.") << 
      ErrorCodeWinLast(load_library_ret.second));
  }

  return load_library_ret.first;
}

void FreeDll(Process const& process, HMODULE module)
{
  Module const kernel32_mod(&process, L"kernel32.dll");
  LPCVOID const free_library = reinterpret_cast<LPCVOID>(
    reinterpret_cast<DWORD_PTR>(FindProcedure(kernel32_mod, "FreeLibrary")));

  typedef BOOL (*FreeLibraryFuncT)(HMODULE hModule);
  std::pair<BOOL, DWORD> const free_library_ret = 
    Call<FreeLibraryFuncT>(process, free_library, CallConv::kWinApi, module);
  if (!free_library_ret.first)
  {
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Call to FreeLibrary in remote process failed.") << 
      ErrorCodeWinLast(free_library_ret.second));
  }
}

// TODO: Configurable timeout.
std::pair<DWORD_PTR, DWORD> CallExport(Process const& process, HMODULE module, 
  std::string const& export_name, LPCVOID export_arg)
{
  Module const module_remote(&process, module);
  LPCVOID const export_ptr = reinterpret_cast<LPCVOID>(
    reinterpret_cast<DWORD_PTR>(FindProcedure(module_remote, export_name)));

  return Call<DWORD_PTR(*)(LPCVOID)>(process, export_ptr, CallConv::kDefault, 
    export_arg);
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
  BOOST_NOEXCEPT
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
  CreateAndInjectData&& other) BOOST_NOEXCEPT
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

CreateAndInjectData::~CreateAndInjectData()
{ }

Process CreateAndInjectData::GetProcess() const
{
  return process_;
}

HMODULE CreateAndInjectData::GetModule() const BOOST_NOEXCEPT
{
  return module_;
}

DWORD_PTR CreateAndInjectData::GetExportRet() const BOOST_NOEXCEPT
{
  return export_ret_;
}

DWORD CreateAndInjectData::GetExportLastError() const BOOST_NOEXCEPT
{
  return export_last_error_;
}

CreateAndInjectData CreateAndInject(
  std::wstring const& path, 
  std::wstring const& work_dir, 
  std::vector<std::wstring> const& args, 
  std::wstring const& module, 
  std::string const& export_name, 
  LPCVOID export_arg, 
  int flags)
{
  boost::filesystem::path const path_real(path);

  std::wstring command_line;
  ArgvQuote(&command_line, path_real.native(), false);
  std::for_each(std::begin(args), std::end(args), 
    [&] (std::wstring const& arg) 
  {
    command_line += L' ';
    ArgvQuote(&command_line, arg, false);
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
  ZeroMemory(&start_info, sizeof(start_info));
  start_info.cb = sizeof(start_info);
  PROCESS_INFORMATION proc_info;
  ZeroMemory(&proc_info, sizeof(proc_info));
  if (!::CreateProcess(path_real.c_str(), proc_args.data(), nullptr, nullptr, 
    FALSE, CREATE_SUSPENDED, nullptr, work_dir_real.c_str(), &start_info, 
    &proc_info))
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Could not create process.") << 
      ErrorCodeWinLast(last_error));
  }

  BOOST_SCOPE_EXIT_ALL(&)
  {
    // WARNING: Handle is leaked if CloseHandle fails.
    BOOST_VERIFY(::CloseHandle(proc_info.hProcess));
    BOOST_VERIFY(::CloseHandle(proc_info.hThread));
  };

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

    LPTHREAD_START_ROUTINE stub_remote_pfn = 
      reinterpret_cast<LPTHREAD_START_ROUTINE>(
      reinterpret_cast<DWORD_PTR>(stub_remote.GetBase()));

    HANDLE const remote_thread = ::CreateRemoteThread(process.GetHandle(), 
      nullptr, 0, stub_remote_pfn, nullptr, 0, nullptr);
    if (!remote_thread)
    {
      DWORD const last_error = ::GetLastError();
      BOOST_THROW_EXCEPTION(HadesMemError() << 
        ErrorString("Could not create remote thread.") << 
        ErrorCodeWinLast(last_error));
    }

    BOOST_SCOPE_EXIT_ALL(&)
    {
      // WARNING: Handle is leaked if CloseHandle fails.
      BOOST_VERIFY(::CloseHandle(remote_thread));
    };

    // TODO: Add a sensible timeout.
    if (::WaitForSingleObject(remote_thread, INFINITE) != WAIT_OBJECT_0)
    {
      DWORD const last_error = ::GetLastError();
      BOOST_THROW_EXCEPTION(HadesMemError() << 
        ErrorString("Could not wait for remote thread.") << 
        ErrorCodeWinLast(last_error));
    }

    HMODULE const remote_module = InjectDll(process, module, flags);

    std::pair<DWORD_PTR, DWORD> export_ret(0, 0);
    if (!export_name.empty())
    {
      // TODO: Configurable timeout.
      export_ret = CallExport(process, remote_module, export_name, export_arg);
    }

    if (::ResumeThread(proc_info.hThread) == static_cast<DWORD>(-1))
    {
      DWORD const last_error = ::GetLastError();
      BOOST_THROW_EXCEPTION(HadesMemError() << 
        ErrorString("Could not resume process.") << 
        ErrorCodeWinLast(last_error) << 
        ErrorCodeWinRet(export_ret.first) << 
        ErrorCodeWinOther(export_ret.second));
    }

    return CreateAndInjectData(process, remote_module, export_ret.first, 
      export_ret.second);
  }
  catch (std::exception const& /*e*/)
  {
    // Terminate process if injection failed, otherwise the 'zombie' process 
    // would be leaked.
    TerminateProcess(proc_info.hProcess, 0);

    throw;
  }
}

}

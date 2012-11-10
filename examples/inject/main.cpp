// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include <array>
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#include <windows.h>
#include <shlwapi.h>
#include <shellapi.h>

#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/injector.hpp>
#include <hadesmem/detail/self_path.hpp>

#include "../common/initialize.hpp"

// TODO: Add support for for passing args, work dir, etc to CreateAndInject.
// e.g. exe-arg0, exe-arg1, exe-arg2, ..., exe-argN?

namespace
{

std::wstring PtrToString(void const* const ptr)
{
  std::wostringstream str;
  str.imbue(std::locale::classic());
  str << std::hex << reinterpret_cast<DWORD_PTR>(ptr);
  return str.str();
}

class CommandLineOpts
{
public:
  CommandLineOpts()
    : help_(false), 
    pid_(0), 
    module_(), 
    path_resolution_(false), 
    export_(), 
    free_(false), 
    add_path_(false), 
    run_()
  { }

  void SetHelp(bool help)
  {
    help_ = help;
  }

  bool GetHelp() const
  {
    return help_;
  }

  void SetPid(DWORD pid)
  {
    pid_ = pid;
  }

  DWORD GetPid() const
  {
    return pid_;
  }

  void SetModule(std::wstring const& module)
  {
    module_ = module;
  }

  std::wstring GetModule() const
  {
    return module_;
  }

  void SetPathResolution(bool path_resolution)
  {
    path_resolution_ = path_resolution;
  }

  bool GetPathResolution() const
  {
    return path_resolution_;
  }

  void SetExport(std::string const& my_export)
  {
    export_ = my_export;
  }

  std::string GetExport() const
  {
    return export_;
  }

  void SetFree(bool free)
  {
    free_ = free;
  }

  bool GetFree() const
  {
    return free_;
  }

  void SetAddPath(bool add_path)
  {
    add_path_ = add_path;
  }

  bool GetAddPath() const
  {
    return add_path_;
  }

  void SetRun(std::wstring const& run)
  {
    run_ = run;
  }

  std::wstring GetRun() const
  {
    return run_;
  }

private:
  bool help_;
  DWORD pid_;
  std::wstring module_;
  bool path_resolution_;
  std::string export_;
  bool free_;
  bool add_path_;
  std::wstring run_;
};

CommandLineOpts ParseCommandLine(int argc, wchar_t** argv)
{
  CommandLineOpts opts;

  for (int i = 1; i < argc; ++i)
  {
    std::wstring const current_arg(argv[i]);
    if (current_arg == L"--help")
    {
      opts.SetHelp(true);
    }
    else if (current_arg == L"--pid")
    {
      if (i + 1 < argc)
      {
        std::wstringstream pid_str(argv[i + 1]);
        DWORD pid = 0;
        pid_str >> pid;
        opts.SetPid(pid);
        ++i;
      }
      else
      {
        BOOST_THROW_EXCEPTION(hadesmem::Error() << 
          hadesmem::ErrorString("Please specify a process ID."));
      }
    }
    else if (current_arg == L"--module")
    {
      if (i + 1 < argc)
      {
        opts.SetModule(argv[i + 1]);
        ++i;
      }
      else
      {
        BOOST_THROW_EXCEPTION(hadesmem::Error() << 
          hadesmem::ErrorString("Please specify a module path."));
      }
    }
    else if (current_arg == L"--path-resolution")
    {
      opts.SetPathResolution(true);
    }
    else if (current_arg == L"--export")
    {
      if (i + 1 < argc)
      {
        std::wstring const export_wide(argv[i + 1]);
        // NOTE: This does a 'dumb' string conversion that is equivalent to 
        // static_cast'ing from wchar_t to char. Under normal circumstances 
        // this would be awful, but in this case the EAT is only allowed to 
        // contain 'ASCII' characters anyway, so using anything other than 
        // ASCII characters in the wide string is a precondition violation.
        opts.SetExport(std::string(std::begin(export_wide), 
          std::end(export_wide)));
        ++i;
      }
      else
      {
        BOOST_THROW_EXCEPTION(hadesmem::Error() << 
          hadesmem::ErrorString("Please specify an export name."));
      }
    }
    else if (current_arg == L"--free")
    {
      opts.SetFree(true);
    }
    else if (current_arg == L"--add-path")
    {
      opts.SetAddPath(true);
    }
    else if (current_arg == L"--run")
    {
      if (i + 1 < argc)
      {
        opts.SetRun(argv[i + 1]);
        ++i;
      }
      else
      {
        BOOST_THROW_EXCEPTION(hadesmem::Error() << 
          hadesmem::ErrorString("Please specify an executable path."));
      }
    }
    else
    {
      BOOST_THROW_EXCEPTION(hadesmem::Error() << 
        hadesmem::ErrorString("Unrecognized argument."));
    }
  }

  return opts;
}

class EnsureLocalFree
{
public:
  explicit EnsureLocalFree(HLOCAL handle)
    : handle_(handle)
  { }

  ~EnsureLocalFree()
  {
    BOOST_VERIFY(!::LocalFree(handle_));
  }

private:
  EnsureLocalFree(EnsureLocalFree const& other) HADESMEM_DELETED_FUNCTION;
  EnsureLocalFree& operator=(EnsureLocalFree const& other) 
    HADESMEM_DELETED_FUNCTION;

  HLOCAL handle_;
};

}

int main()
{
  try
  {
    DisableUserModeCallbackExceptionFilter();
    EnableCrtDebugFlags();
    EnableTerminationOnHeapCorruption();
    EnableBottomUpRand();

    std::cout << "HadesMem Injector\n";

    int argc = 0;
    LPWSTR* argv = ::CommandLineToArgvW(::GetCommandLine(), &argc);
    if (!argv)
    {
      DWORD const last_error = ::GetLastError();
      BOOST_THROW_EXCEPTION(hadesmem::Error() << 
        hadesmem::ErrorString("CommandLineToArgvW failed.") << 
        hadesmem::ErrorCodeWinLast(last_error));
    }
    EnsureLocalFree ensure_free_command_line(reinterpret_cast<HLOCAL>(argv));

    CommandLineOpts const opts(ParseCommandLine(argc, argv));

    if (opts.GetHelp() || argc == 1)
    {
      std::cout << 
        "\nOptions:\n"
        "  --help\t\t\tproduce help message\n"
        "  --pid arg\t\t\tprocess id\n"
        "  --module arg\t\t\tmodule path\n"
        "  --path-resolution\t\tperform path resolution\n"
        "  --export arg\t\t\texport name\n"
        "  --free\t\t\tunload module\n"
        "  --add-path\t\t\tadd module dir to search order\n"
        "  --run arg\t\t\tprocess path\n";

      return 1;
    }

    if (!opts.GetModule().size())
    {
      std::cerr << "\nError! Module path must be specified.\n";
      return 1;
    }
    
    bool const has_pid = opts.GetPid() != 0;
    bool const create_proc = !opts.GetRun().empty();
    if ((has_pid && create_proc) || (!has_pid && !create_proc))
    {
      std::cerr << "\nError! A process ID or an executable path must be "
        "specified.\n";
      return 1;
    }

    bool const inject = !opts.GetFree();
    if (!inject && create_proc)
    {
      std::cerr << "\nError! Modules can only be unloaded from running "
        "targets.\n";
      return 1;
    }

    std::wstring const module_path = opts.GetModule();
    bool const path_resolution = opts.GetPathResolution();
    bool const add_path = opts.GetAddPath();

    int flags = hadesmem::InjectFlags::kNone;
    if (path_resolution)
    {
      flags |= hadesmem::InjectFlags::kPathResolution;
    }
    if (add_path)
    {
      flags |= hadesmem::InjectFlags::kAddToSearchOrder;
    }

    if (has_pid)
    {
      try
      {
        hadesmem::GetSeDebugPrivilege();

        std::wcout << "\nAcquired SeDebugPrivilege.\n";
      }
      catch (std::exception const& /*e*/)
      {
        std::wcout << "\nFailed to acquire SeDebugPrivilege.\n";
      }

      DWORD const pid = opts.GetPid();

      hadesmem::Process const process(pid);
      
      HMODULE module = nullptr;

      if (inject)
      {
        module = hadesmem::InjectDll(process, module_path, flags);

        std::wcout << "\nSuccessfully injected module at base address " << 
          PtrToString(module) << ".\n";
      }
      else
      {
        std::wstring path_real(module_path);
        if (path_resolution && ::PathIsRelative(path_real.c_str()))
        {
          std::array<wchar_t, MAX_PATH> absolute_path = { { 0 } };
          if (!::PathCombine(absolute_path.data(), 
            hadesmem::detail::GetSelfDirPath().c_str(), 
            path_real.c_str()))
          {
            DWORD const last_error = ::GetLastError();
            BOOST_THROW_EXCEPTION(hadesmem::Error() << 
              hadesmem::ErrorString("PathCombine failed.") << 
              hadesmem::ErrorCodeWinLast(last_error));
          }
          path_real = absolute_path.data();
        }

        hadesmem::Module const remote_module(&process, path_real);
        module = remote_module.GetHandle();
      }

      if (opts.GetExport().size())
      {
        std::string const export_name = opts.GetExport();
        auto const export_ret = hadesmem::CallExport(process, module, 
          export_name);

        std::wcout << "\nSuccessfully called module export.\n";
        std::wcout << "Return: " << export_ret.GetReturnValue() << ".\n";
        std::wcout << "LastError: " << export_ret.GetLastError() << ".\n";
      }

      if (!inject)
      {
        hadesmem::FreeDll(process, module);

        std::wcout << "\nSuccessfully freed module at base address " << 
          PtrToString(module) << ".\n";
      }
    }
    else
    {
      std::wstring const exe_path = opts.GetRun();
      std::string const export_name = opts.GetExport();
      hadesmem::CreateAndInjectData const inject_data = 
        hadesmem::CreateAndInject(exe_path, L"", std::vector<std::wstring>(), 
        module_path, export_name, flags);

      std::wcout << "\nSuccessfully created target.\n";
      std::wcout << "Process ID: " << inject_data.GetProcess() << ".\n";
      std::wcout << "Module Base: " << PtrToString(inject_data.GetModule()) 
        << ".\n";
      std::wcout << "Export Return: " << inject_data.GetExportRet() << ".\n";
      std::wcout << "Export LastError: " << inject_data.GetExportLastError() 
        << ".\n";
    }

    return 0;
  }
  catch (std::exception const& e)
  {
    std::cerr << "\nError!\n";
    std::cerr << boost::diagnostic_information(e) << "\n";

    return 1;
  }
}

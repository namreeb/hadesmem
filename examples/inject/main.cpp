// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include <memory>
#include <string>
#include <vector>
#include <iostream>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/error.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/injector.hpp>
#include <hadesmem/detail/self_path.hpp>

#include "../common/initialize.hpp"

// TODO: Add support for for passing args, work dir, etc to CreateAndInject.
// e.g. exe-arg0, exe-arg1, exe-arg2, ..., exe-argN?
// TODO: Fix flag names (--run, etc).
// TODO: Add flag to keep process paused after creation for debugging.

namespace
{

std::wstring PtrToString(void const* const ptr)
{
  std::wostringstream str;
  str.imbue(std::locale::classic());
  str << std::hex << reinterpret_cast<DWORD_PTR>(ptr);
  return str.str();
}

}

int main()
{
  try
  {
    DisableUserModeCallbackExceptionFilter();
    EnableCrtDebugFlags();
    EnableTerminationOnHeapCorruption();
    EnableBottomUpRand();
    ImbueAllDefault();

    std::cout << "HadesMem Injector\n";

    boost::program_options::options_description opts_desc(
      "General options");
    opts_desc.add_options()
      ("help", "produce help message")
      ("pid", boost::program_options::value<DWORD>(), "process id")
      ("module", boost::program_options::wvalue<std::wstring>(), "module path")
      ("path-resolution", "perform path resolution")
      ("export", boost::program_options::value<std::string>(), "export name")
      ("free", "unload module")
      ("add-path", "add module dir to serach order")
      ("exe-path", boost::program_options::wvalue<std::wstring>(), 
      "process exe path")
      ;

    std::vector<std::wstring> const args = boost::program_options::
      split_winmain(GetCommandLine());
    boost::program_options::variables_map var_map;
    boost::program_options::store(boost::program_options::wcommand_line_parser(
      args).options(opts_desc).run(), var_map);
    boost::program_options::notify(var_map);

    if (var_map.count("help"))
    {
      std::cout << '\n' << opts_desc << '\n';
      return 1;
    }

    if (!var_map.count("module"))
    {
      std::cerr << "Error! Module path must be specified.\n";
      return 1;
    }
    
    bool const has_pid = var_map.count("pid") != 0;
    bool const has_exe_path = var_map.count("exe-path") != 0;
    if ((has_pid && has_exe_path) || (!has_pid && !has_exe_path))
    {
      std::cerr << "Error! A process ID or an executable path must be "
        "specified.\n";
      return 1;
    }

    bool const inject = var_map.count("free") == 0;
    if (!inject && has_exe_path)
    {
      std::cerr << "Error! Modules can only be unloaded from running "
        "targets.\n";
      return 1;
    }

    std::wstring const module_path = var_map["module"].as<std::wstring>();
    bool const path_resolution = var_map.count("path-resolution") != 0;
    bool const add_path = var_map.count("add-path") != 0;

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

      DWORD const pid = var_map["pid"].as<DWORD>();

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
        boost::filesystem::path path_real(module_path);
        if (path_resolution && path_real.is_relative())
        {
          path_real = boost::filesystem::absolute(path_real, 
            hadesmem::detail::GetSelfDirPath());
        }
        path_real.make_preferred();

        hadesmem::Module const remote_module(&process, path_real.native());
        module = remote_module.GetHandle();
      }

      if (var_map.count("export"))
      {
        std::string const export_name = var_map["export"].as<std::string>();
        std::pair<DWORD_PTR, DWORD> const export_ret = hadesmem::CallExport(
          process, module, export_name, nullptr);

        std::wcout << "Successfully called module export.\n";
        std::wcout << "Return: " << export_ret.first << ".\n";
        std::wcout << "LastError: " << export_ret.second << ".\n";
      }

      if (!inject)
      {
        hadesmem::FreeDll(process, module);
      }
    }
    else
    {
      std::wstring const exe_path = var_map["exe-path"].as<std::wstring>();
      std::string const export_name = var_map.count("export") ? 
        var_map["export"].as<std::string>() : "";
      hadesmem::CreateAndInjectData const inject_data = 
        hadesmem::CreateAndInject(exe_path, L"", std::vector<std::wstring>(), 
        module_path, export_name, nullptr, flags);
    }
  }
  catch (std::exception const& e)
  {
    std::cerr << "Error!\n";
    std::cerr << boost::diagnostic_information(e) << "\n";

    return 1;
  }
}

// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <windows.h>

#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/injector.hpp>
#include <hadesmem/detail/self_path.hpp>

// TODO: Don't use a relative path
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

}

int main(int argc, char* /*argv*/[]) 
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
      ("run", boost::program_options::wvalue<std::wstring>(), "process path")
      ;

    std::vector<std::wstring> const args = boost::program_options::
      split_winmain(GetCommandLine());
    boost::program_options::variables_map var_map;
    boost::program_options::store(boost::program_options::wcommand_line_parser(
      args).options(opts_desc).run(), var_map);
    boost::program_options::notify(var_map);

    if (var_map.count("help") || argc == 1)
    {
      std::cout << '\n' << opts_desc << '\n';
      return 1;
    }

    if (!var_map.count("module"))
    {
      std::cerr << "\nError! Module path must be specified.\n";
      return 1;
    }
    
    bool const has_pid = (var_map.count("pid") != 0);
    bool const create_proc = (var_map.count("run") != 0);
    if ((has_pid && create_proc) || (!has_pid && !create_proc))
    {
      std::cerr << "\nError! A process ID or an executable path must be "
        "specified.\n";
      return 1;
    }

    bool const inject = (var_map.count("free") == 0);
    if (!inject && create_proc)
    {
      std::cerr << "\nError! Modules can only be unloaded from running "
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
      std::wstring const exe_path = var_map["run"].as<std::wstring>();
      std::string const export_name = var_map.count("export") ? 
        var_map["export"].as<std::string>() : "";
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
    std::cerr << boost::diagnostic_information(e) << '\n';

    return 1;
  }
}

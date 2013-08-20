// Copyright (C) 2010-2013 Joshua Boyce.
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

#include <hadesmem/call.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/injector.hpp>
#include <hadesmem/process_list.hpp>
#include <hadesmem/process_entry.hpp>
#include <hadesmem/detail/self_path.hpp>
#include <hadesmem/detail/initialize.hpp>
#include <hadesmem/detail/to_upper_ordinal.hpp>

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
    hadesmem::detail::InitializeAll();

    std::cout << "HadesMem Injector\n";

    boost::program_options::options_description opts_desc(
      "General options");
    opts_desc.add_options()
      ("help", "produce help message")
      ("pid", boost::program_options::value<DWORD>(), "process id")
      ("name", boost::program_options::wvalue<std::wstring>(), "process name")
      ("module", boost::program_options::wvalue<std::wstring>(), "module path")
      ("path-resolution", "perform path resolution")
      ("export", boost::program_options::value<std::string>(), "export name")
      ("inject", "inject module")
      ("free", "unload module")
      ("add-path", "add module dir to search order")
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
    bool const has_name = (var_map.count("name") != 0);
    if ((has_pid && create_proc) || (has_pid && has_name) || 
      (create_proc && has_name))
    {
      std::cerr << "\nError! A process ID, process name, or executable path "
        "must be specified, not a combination.\n";
      return 1;
    }

    if (!has_pid && !has_name && !create_proc)
    {
      std::cerr << "\nError! A process ID, process name, or executable path "
        "must be specified.\n";
    }

    bool const inject = (var_map.count("inject") != 0);
    bool const free = (var_map.count("free") != 0);
    bool const call_export = (var_map.count("export") != 0);

    if (inject && free)
    {
      std::cerr << "\nError! Please specify inject or free, not both.\n";
    }

    if (free && create_proc)
    {
      std::cerr << "\nError! Modules can only be unloaded from running "
        "targets.\n";
      return 1;
    }

    if (!inject && create_proc)
    {
      std::cerr << "\nError! Exports can only be called without injection on "
        "running targets. Did you mean to use --inject?\n";
      return 1;
    }

    if (!inject && !free && !call_export)
    {
      std::cerr << "\nError! Please choose action(s) to perform on the "
        "process (inject, free, export).\n";
      return 1;
    }

    std::wstring const module_path = var_map["module"].as<std::wstring>();
    std::wstring const proc_name = var_map["name"].as<std::wstring>();
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

    if (has_pid || has_name)
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

      std::unique_ptr<hadesmem::Process> process;

      if (has_pid)
      {
        DWORD const pid = var_map["pid"].as<DWORD>();
        process.reset(new hadesmem::Process(pid));
      }
      else
      {
        std::wstring const proc_name_upper = hadesmem::detail::ToUpperOrdinal(
          proc_name);
        hadesmem::ProcessList proc_list;
        auto iter = std::find_if(std::begin(proc_list), std::end(proc_list), 
          [&] (hadesmem::ProcessEntry const& proc_entry)
        {
          return hadesmem::detail::ToUpperOrdinal(proc_entry.GetName()) == 
            proc_name_upper;
        });
        if (iter != std::end(proc_list))
        {
          process.reset(new hadesmem::Process(iter->GetId()));
        }
        else
        {
          std::wcerr << "\nError! Failed to find process \"" << proc_name 
            << "\".\n";
          return 1;
        }
      }
      
      HMODULE module = nullptr;

      if (inject)
      {
        module = hadesmem::InjectDll(*process, module_path, flags);

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

        hadesmem::Module const remote_module(*process, path_real.native());
        module = remote_module.GetHandle();
      }

      if (call_export)
      {
        std::string const export_name = var_map["export"].as<std::string>();
        auto const export_ret = hadesmem::CallExport(*process, module, 
          export_name);

        std::wcout << "\nSuccessfully called module export.\n";
        std::wcout << "Return: " << export_ret.GetReturnValue() << ".\n";
        std::wcout << "LastError: " << export_ret.GetLastError() << ".\n";
      }

      if (free)
      {
        hadesmem::FreeDll(*process, module);

        std::wcout << "\nSuccessfully freed module at base address " << 
          PtrToString(module) << ".\n";
      }
    }
    else
    {
      std::vector<std::wstring> create_args;
      std::wstring const exe_path = var_map["run"].as<std::wstring>();
      std::string const export_name = var_map.count("export") ? 
        var_map["export"].as<std::string>() : "";
      hadesmem::CreateAndInjectData const inject_data = 
        hadesmem::CreateAndInject(
        exe_path, 
        L"", 
        std::begin(create_args), 
        std::end(create_args), 
        module_path, 
        export_name, 
        flags);

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

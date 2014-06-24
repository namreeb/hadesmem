// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include <algorithm>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <windows.h>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <tclap/CmdLine.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/call.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/debug_privilege.hpp>
#include <hadesmem/detail/filesystem.hpp>
#include <hadesmem/detail/self_path.hpp>
#include <hadesmem/detail/str_conv.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/injector.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/process_helpers.hpp>

int main(int argc, char* argv[])
{
  try
  {
    std::cout << "HadesMem Injector [" << HADESMEM_VERSION_STRING << "]\n";

    TCLAP::CmdLine cmd{"DLL injector", ' ', HADESMEM_VERSION_STRING};
    TCLAP::ValueArg<DWORD> pid_arg{
      "", "pid", "Target process id", false, 0, "DWORD"};
    TCLAP::ValueArg<std::string> name_arg{
      "", "name", "Target process name", false, "", "string"};
    TCLAP::ValueArg<std::string> run_arg{
      "", "run", "Target process path (new instance)", false, "", "string"};
    std::vector<TCLAP::Arg*> xor_args{&pid_arg, &name_arg, &run_arg};
    cmd.xorAdd(xor_args);
    TCLAP::SwitchArg name_forced_arg{
      "",
      "name-forced",
      "Default to first matched process name (no warning)",
      cmd};
    TCLAP::ValueArg<std::string> module_arg{
      "", "module", "Module path", true, "", "string", cmd};
    TCLAP::SwitchArg path_resolution_arg{
      "",
      "path-resolution",
      "Perform (local) path resolution on module path",
      cmd};
    TCLAP::SwitchArg add_path_arg{
      "", "add-path", "Add module dir to (remote) search order", cmd};
    TCLAP::ValueArg<std::string> export_arg{
      "",
      "export",
      "Module export name (DWORD_PTR (*) ())",
      false,
      "",
      "string",
      cmd};
    TCLAP::SwitchArg inject_arg{"", "inject", "Inject module"};
    TCLAP::SwitchArg free_arg{"", "free", "Free module"};
    cmd.xorAdd(inject_arg, free_arg);
    TCLAP::MultiArg<std::string> arg_arg{
      "",
      "arg",
      "Target process args (use once for each arg)",
      false,
      "string",
      cmd};
    TCLAP::ValueArg<std::string> work_dir_arg{
      "",
      "work-dir",
      "Target process working directory",
      false,
      "",
      "string",
      cmd};
    cmd.parse(argc, argv);

    bool const free = free_arg.isSet();
    bool const run = run_arg.isSet();
    if (free && run)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{
          "Modules can only be unloaded from running targets."});
    }

    bool const inject = inject_arg.isSet();
    if (!inject && run)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error{}
                                      << hadesmem::ErrorString{
                                        "Exports can only be called without "
                                        "injection on running targets. Did "
                                        "you mean to use --inject?"});
    }

    bool const call_export = export_arg.isSet();
    if (!inject && !free && !call_export)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"Please choose action(s) to "
                                                   "perform on the process "
                                                   "(inject, free, export)."});
    }

    auto const module_path =
      hadesmem::detail::MultiByteToWideChar(module_arg.getValue());
    bool const path_resolution = path_resolution_arg.isSet();
    bool const add_path = add_path_arg.isSet();

    std::uint32_t flags = hadesmem::InjectFlags::kNone;
    if (path_resolution)
    {
      flags |= hadesmem::InjectFlags::kPathResolution;
    }
    if (add_path)
    {
      flags |= hadesmem::InjectFlags::kAddToSearchOrder;
    }

    bool const has_pid = pid_arg.isSet();
    bool const has_name = name_arg.isSet();
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
        DWORD const pid = pid_arg.getValue();
        process = std::make_unique<hadesmem::Process>(pid);
      }
      else
      {
        auto const proc_name =
          hadesmem::detail::MultiByteToWideChar(name_arg.getValue());
        bool const name_forced = name_forced_arg.isSet();
        process = std::make_unique<hadesmem::Process>(
          hadesmem::GetProcessByName(proc_name, name_forced));
      }

      HMODULE module = nullptr;

      if (inject)
      {
        module = hadesmem::InjectDll(*process, module_path, flags);

        std::wcout << "\nSuccessfully injected module at base "
                      "address " << hadesmem::detail::PtrToHexString(module)
                   << ".\n";
      }
      else
      {
        std::wstring path_real{module_path};
        if (path_resolution && hadesmem::detail::IsPathRelative(path_real))
        {
          path_real = hadesmem::detail::CombinePath(
            hadesmem::detail::GetSelfDirPath(), path_real);
        }

        hadesmem::Module const remote_module{*process, path_real};
        module = remote_module.GetHandle();
      }

      if (call_export)
      {
        auto const export_name = export_arg.getValue();
        hadesmem::CallResult<DWORD_PTR> const export_ret =
          hadesmem::CallExport(*process, module, export_name);

        std::wcout << "\nSuccessfully called module export.\n";
        std::wcout << "Return: " << export_ret.GetReturnValue() << ".\n";
        std::wcout << "LastError: " << export_ret.GetLastError() << ".\n";
      }

      if (free)
      {
        hadesmem::FreeDll(*process, module);

        std::wcout << "\nSuccessfully freed module at base address "
                   << hadesmem::detail::PtrToHexString(module) << ".\n";
      }
    }
    else
    {
      auto const exe_path =
        hadesmem::detail::MultiByteToWideChar(run_arg.getValue());
      auto const exe_args_tmp = arg_arg.getValue();
      std::vector<std::wstring> exe_args;
      std::transform(std::begin(exe_args_tmp),
                     std::end(exe_args_tmp),
                     std::back_inserter(exe_args),
                     [](std::string const& s)
                     {
        return hadesmem::detail::MultiByteToWideChar(s);
      });
      auto const export_name = export_arg.getValue();
      auto const work_dir =
        hadesmem::detail::MultiByteToWideChar(work_dir_arg.getValue());
      hadesmem::CreateAndInjectData const inject_data =
        hadesmem::CreateAndInject(exe_path,
                                  work_dir,
                                  std::begin(exe_args),
                                  std::end(exe_args),
                                  module_path,
                                  export_name,
                                  flags);

      std::wcout << "\nSuccessfully created target.\n";
      std::wcout << "Process ID: " << inject_data.GetProcess() << ".\n";
      std::wcout << "Module Base: " << hadesmem::detail::PtrToHexString(
                                         inject_data.GetModule()) << ".\n";
      std::wcout << "Export Return: " << inject_data.GetExportRet() << ".\n";
      std::wcout << "Export LastError: " << inject_data.GetExportLastError()
                 << ".\n";
    }

    return 0;
  }
  catch (...)
  {
    std::cerr << "\nError!\n";
    std::cerr << boost::current_exception_diagnostic_information() << '\n';

    return 1;
  }
}

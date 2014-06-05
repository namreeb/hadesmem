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
#include <hadesmem/detail/to_upper_ordinal.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/injector.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/process_entry.hpp>
#include <hadesmem/process_list.hpp>

namespace
{

DWORD FindProc(std::wstring const& proc_name, bool name_forced)
{
  std::wstring const proc_name_upper =
    hadesmem::detail::ToUpperOrdinal(proc_name);
  auto const compare_proc_name = [&](hadesmem::ProcessEntry const& proc_entry)
  {
    return hadesmem::detail::ToUpperOrdinal(proc_entry.GetName()) ==
           proc_name_upper;
  };
  hadesmem::ProcessList proc_list;
  if (name_forced)
  {
    auto const proc_iter = std::find_if(
      std::begin(proc_list), std::end(proc_list), compare_proc_name);
    if (proc_iter == std::end(proc_list))
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"Failed to find process."});
    }

    return proc_iter->GetId();
  }
  else
  {
    std::vector<hadesmem::ProcessEntry> found_procs;
    std::copy_if(std::begin(proc_list),
                 std::end(proc_list),
                 std::back_inserter(found_procs),
                 compare_proc_name);

    if (found_procs.empty())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"Failed to find process."});
    }

    if (found_procs.size() > 1)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{
          "Process name search found multiple matches. "
          "Please specify a PID or use --name-forced."});
    }

    return found_procs.front().GetId();
  }
}
}

int main(int argc, char* argv[])
{
  try
  {
    std::cout << "HadesMem Injector [" << HADESMEM_VERSION_STRING << "]\n";

    TCLAP::CmdLine cmd{"DLL injector", ' ', HADESMEM_VERSION_STRING};
    TCLAP::ValueArg<DWORD> pid_arg{
      "p", "pid", "Target process id", false, 0, "DWORD"};
    TCLAP::ValueArg<std::string> name_arg{
      "n", "name", "Target process name", false, "", "string"};
    TCLAP::ValueArg<std::string> run_arg{
      "r", "run", "Target process path (new instance)", false, "", "string"};
    std::vector<TCLAP::Arg*> xor_args{&pid_arg, &name_arg, &run_arg};
    cmd.xorAdd(xor_args);
    TCLAP::SwitchArg name_forced_arg{
      "",
      "name-forced",
      "Default to first matched process name (no warning)",
      cmd};
    TCLAP::ValueArg<std::string> module_arg{
      "m", "module", "Module path", true, "", "string", cmd};
    TCLAP::SwitchArg path_resolution_arg{
      "",
      "path-resolution",
      "Perform (local) path resolution on module path",
      cmd};
    TCLAP::SwitchArg add_path_arg{
      "", "add-path", "Add module dir to (remote) search order", cmd};
    TCLAP::ValueArg<std::string> export_arg{
      "e",
      "export",
      "Module export name (DWORD_PTR (*) ())",
      false,
      "",
      "string",
      cmd};
    TCLAP::SwitchArg inject_arg{"i", "inject", "Inject module"};
    TCLAP::SwitchArg free_arg{"f", "free", "Free module"};
    cmd.xorAdd(inject_arg, free_arg);
    TCLAP::MultiArg<std::string> arg_arg{
      "a",
      "arg",
      "Target process args (use once for each arg)",
      false,
      "string",
      cmd};
    TCLAP::ValueArg<std::string> work_dir_arg{
      "w",
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

        // Guard against potential PID reuse race condition. Unlikely
        // to ever happen in practice, but better safe than sorry.
        DWORD proc_pid = 0;
        DWORD proc_pid_2 = 0;
        DWORD retries = 3;
        do
        {
          proc_pid = FindProc(proc_name, name_forced);
          process = std::make_unique<hadesmem::Process>(proc_pid);

          proc_pid_2 = FindProc(proc_name, name_forced);
          hadesmem::Process process_2{proc_pid_2};
        } while (proc_pid != proc_pid_2 && retries--);

        if (proc_pid != proc_pid_2)
        {
          HADESMEM_DETAIL_THROW_EXCEPTION(
            hadesmem::Error{} << hadesmem::ErrorString{
              "Could not get handle to target process (PID reuse race)."});
        }
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

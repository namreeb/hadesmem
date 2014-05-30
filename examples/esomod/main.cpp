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

#include <hadesmem/config.hpp>
#include <hadesmem/debug_privilege.hpp>
#include <hadesmem/detail/to_upper_ordinal.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/find_pattern.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/process_entry.hpp>
#include <hadesmem/process_list.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

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
    std::cout << "HadesMem ESO Mod [" << HADESMEM_VERSION_STRING << "]\n";

    TCLAP::CmdLine cmd{"ESO Mod", ' ', HADESMEM_VERSION_STRING};
    TCLAP::ValueArg<float> fov_arg{
      "f", "fov", "Field of View (in degrees)", true, 50.0f, "float", cmd};
    cmd.parse(argc, argv);

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

    auto const proc_name = L"eso.exe";

    // Guard against potential PID reuse race condition. Unlikely
    // to ever happen in practice, but better safe than sorry.
    DWORD proc_pid = 0;
    DWORD proc_pid_2 = 0;
    DWORD retries = 3;
    do
    {
      proc_pid = FindProc(proc_name, false);
      process = std::make_unique<hadesmem::Process>(proc_pid);

      proc_pid_2 = FindProc(proc_name, false);
      hadesmem::Process process_2{proc_pid_2};
    } while (proc_pid != proc_pid_2 && retries--);

    if (proc_pid != proc_pid_2)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{
          "Could not get handle to target process (PID reuse race)."});
    }

    // eso.live.1.1.2.995904 (dumped with module base of 0x00960000)
    // .text:00A58435                 jnz     loc_A58641
    // .text:00A5843B                 mov     edx, dword_1BCA930
    auto const camera_manager_ref = static_cast<std::uint8_t*>(hadesmem::Find(
      *process,
      L"",
      L"0x0F 0x85 ?? ?? ?? ?? 0x8B 0x15 ?? ?? ?? ?? 0x8B 0x4A 0x14",
      hadesmem::PatternFlags::kThrowOnUnmatch,
      0));
    std::cout << "Got camera manager ref. ["
              << static_cast<void*>(camera_manager_ref) << "].\n";

    auto const kCameraManagerRefOffset = 0x08;
    auto const camera_manager_ptr = hadesmem::Read<std::uint8_t*>(
      *process, camera_manager_ref + kCameraManagerRefOffset);
    std::cout << "Got camera manager ptr. ["
              << static_cast<void*>(camera_manager_ptr) << "].\n";

    auto const camera_manager =
      hadesmem::Read<std::uint8_t*>(*process, camera_manager_ptr);
    std::cout << "Got camera manager. [" << static_cast<void*>(camera_manager)
              << "].\n";

    // eso.live.1.1.2.995904 (dumped with module base of 0x00960000)
    // .text:00A72AB0                 cmp     dword ptr [ecx+14h], 0
    auto const kCameraOffset = 0x14;
    auto const camera =
      hadesmem::Read<std::uint8_t*>(*process, camera_manager + kCameraOffset);
    std::cout << "Got camera. [" << static_cast<void*>(camera) << "].\n";

    auto const new_fov = fov_arg.getValue() * 0.01745327934622765f;
    std::cout << "New FoV is " << new_fov << ".\n";

    // eso.live.1.1.2.995904 (dumped with module base of 0x00960000)
    // .text:00A780C6                 fadd    dword ptr [esi+478h]
    // eso.live.1.1.2.995904 (dumped with module base of 0x00960000)
    // .text:00A79620                 fadd    dword ptr [esi+478h]
    auto const kCameraVertFov3pOffset = 0x478;
    auto const vert_fov_3p = camera + kCameraVertFov3pOffset;
    std::cout << "Writing 3rd person FoV. [" << static_cast<void*>(vert_fov_3p)
              << "].\n";
    hadesmem::Write(*process, vert_fov_3p, new_fov);

    // eso.live.1.1.2.995904 (dumped with module base of 0x00960000)
    // .text:00A780B5                 fld     dword ptr [esi+47Ch]
    // eso.live.1.1.2.995904 (dumped with module base of 0x00960000)
    // .text:00A794D4                 fld     dword ptr [esi+47Ch]
    auto const kCameraVertFov1pOffset = 0x47C;
    auto const vert_fov_1p = camera + kCameraVertFov1pOffset;
    std::cout << "Writing 1st person FoV. [" << static_cast<void*>(vert_fov_1p)
              << "].\n";
    hadesmem::Write(*process, vert_fov_1p, new_fov);

    std::cout << "Finished.\n";

    return 0;
  }
  catch (...)
  {
    std::cerr << "\nError!\n";
    std::cerr << boost::current_exception_diagnostic_information() << '\n';

    return 1;
  }
}

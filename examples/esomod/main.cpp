// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include <algorithm>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include <windows.h>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <tclap/cmdline.h>
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

#include "3d.hpp"
#include "camera_distance.hpp"
#include "fader.hpp"
#include "fog.hpp"
#include "fov.hpp"
#include "tone_mapping.hpp"
#include "view_distances.hpp"
#include "watermark.hpp"

namespace
{

DWORD FindProc(std::wstring const& proc_name)
{
  std::wstring const proc_name_upper =
    hadesmem::detail::ToUpperOrdinal(proc_name);
  auto const compare_proc_name = [&](hadesmem::ProcessEntry const& proc_entry)
  {
    return hadesmem::detail::ToUpperOrdinal(proc_entry.GetName()) ==
           proc_name_upper;
  };
  hadesmem::ProcessList proc_list;
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
        "Please use --pid."});
  }

  return found_procs.front().GetId();
}
}

int main(int argc, char* argv[])
{
  try
  {
    std::cout << "HadesMem ESO Mod [" << HADESMEM_VERSION_STRING << "]\n";

    TCLAP::CmdLine cmd{"ESO Mod", ' ', HADESMEM_VERSION_STRING};
    TCLAP::ValueArg<DWORD> pid_arg{"",
                                   "pid",
                                   "Process ID (for multiple ESO instances)",
                                   false,
                                   0,
                                   "DWORD",
                                   cmd};
    // This should probably have some value constraints set, but it's more
    // entertaining to leave it open so people can set it to silly values.
    TCLAP::ValueArg<float> fov_arg{
      "", "fov", "Set field of view (in degrees)", false, 50.0f, "float", cmd };
    TCLAP::ValueArg<float> max_camera_distance_arg{
      "", "max-camera-dist", "Set max camera distance", false, 10.0f, "float", cmd };
    // Values above 5 seem to be invalid, but leave it open in case that changes
    // in the future (that way if the patterns don't need updating I don't need
    // to do anything).
    TCLAP::ValueArg<std::uint32_t> tone_mapping_arg{
      "",
      "tone-mapping",
      "Set tone mapping type (a.k.a shader filters)",
      false,
      0,
      "uint32_t",
      cmd };
    TCLAP::ValueArg<float> min_view_distance_arg{"",
                                                 "min-view-dist",
                                                 "Set minimum view distance",
                                                 false,
                                                 0.40000001f,
                                                 "float",
                                                 cmd};
    TCLAP::ValueArg<float> max_view_distance_arg{"",
                                                 "max-view-dist",
                                                 "Set maximum view distance",
                                                 false,
                                                 2.0f,
                                                 "float",
                                                 cmd};
    TCLAP::SwitchArg fog_arg{"", "fog", "Toggle fog", cmd};
    TCLAP::SwitchArg anaglyph_arg{"", "3d", "Toggle anaglyph 3D", cmd};
    TCLAP::SwitchArg watermark_arg{"", "watermark", "Toggle watermark", cmd};
    TCLAP::SwitchArg fader_arg{
      "", "fader", "Toggle fader (force high quality models)", cmd};
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

    if (pid_arg.isSet())
    {
      DWORD const pid = pid_arg.getValue();
      process = std::make_unique<hadesmem::Process>(pid);
    }
    else
    {
      // Guard against potential PID reuse race condition. Unlikely
      // to ever happen in practice, but better safe than sorry.
      DWORD proc_pid = 0;
      DWORD proc_pid_2 = 0;
      std::uint32_t retries = 3;
      do
      {
        std::wstring const kProcName = L"eso.exe";

        proc_pid = FindProc(kProcName);
        process = std::make_unique<hadesmem::Process>(proc_pid);

        proc_pid_2 = FindProc(kProcName);
        hadesmem::Process process_2{proc_pid_2};
      } while (proc_pid != proc_pid_2 && retries--);

      if (proc_pid != proc_pid_2)
      {
        HADESMEM_DETAIL_THROW_EXCEPTION(
          hadesmem::Error{} << hadesmem::ErrorString{
            "Could not get handle to target process (PID reuse race)."});
      }
    }

    if (fov_arg.isSet())
    {
      SetFov(*process, fov_arg.getValue());
    }

    if (max_camera_distance_arg.isSet())
    {
      SetMaxCameraDistance(*process, max_camera_distance_arg.getValue());
    }

    if (tone_mapping_arg.isSet())
    {
      SetToneMappingType(*process, tone_mapping_arg.getValue());
    }

    bool const set_min_view_distance = min_view_distance_arg.isSet();
    bool const set_max_view_distance = max_view_distance_arg.isSet();
    if (set_min_view_distance || set_max_view_distance)
    {
      auto const min_view_distance =
        set_min_view_distance ? &min_view_distance_arg.getValue() : nullptr;
      auto const max_view_distance =
        set_max_view_distance ? &max_view_distance_arg.getValue() : nullptr;
      SetViewDistances(*process, min_view_distance, max_view_distance);
    }

    if (fog_arg.isSet())
    {
      ToggleFog(*process);
    }

    if (anaglyph_arg.isSet())
    {
      Toggle3D(*process);
    }

    if (watermark_arg.isSet())
    {
      ToggleWatermark(*process);
    }

    if (fader_arg.isSet())
    {
      ToggleFader(*process);
    }

    std::cout << "\nFinished.\n";

    return 0;
  }
  catch (...)
  {
    std::cerr << "\nError!\n";
    std::cerr << boost::current_exception_diagnostic_information() << '\n';

    return 1;
  }
}

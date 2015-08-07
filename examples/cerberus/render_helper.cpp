// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "render_helper.hpp"

#include <iterator>
#include <string>
#include <vector>

#include <windows.h>

#include <hadesmem/detail/filesystem.hpp>
#include <hadesmem/detail/smart_handle.hpp>
#include <hadesmem/detail/trace.hpp>

#include "main.hpp"
#include "hook_disabler.hpp"
#include "process.hpp"

namespace
{
hadesmem::cerberus::RenderOffsets& GetRenderOffsetsImpl() noexcept
{
  static hadesmem::cerberus::RenderOffsets offsets;
  return offsets;
}

class RenderHelperImpl : public hadesmem::cerberus::RenderHelperInterface
{
public:
  virtual hadesmem::cerberus::RenderOffsets const* GetRenderOffsets() final
  {
    return &GetRenderOffsetsImpl();
  }
};
}

namespace hadesmem
{
namespace cerberus
{
RenderHelperInterface& GetRenderHelperInterface() noexcept
{
  static RenderHelperImpl render_helper_impl;
  return render_helper_impl;
}

void InitializeRenderHelper()
{
  auto const pid_str = std::to_wstring(::GetCurrentProcessId());

  auto const file_mapping_name = GenerateRenderHelperMapName(pid_str);
  HADESMEM_DETAIL_TRACE_FORMAT_W(L"Helper mapping name: [%s].",
                                 file_mapping_name.c_str());
  hadesmem::detail::SmartHandle file_mapping{
    ::CreateFileMappingW(INVALID_HANDLE_VALUE,
                         nullptr,
                         PAGE_READWRITE,
                         0,
                         sizeof(hadesmem::cerberus::RenderOffsets),
                         file_mapping_name.c_str())};
  if (!file_mapping.IsValid())
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"CreateFileMappingW failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  hadesmem::detail::SmartMappedFileHandle mapping_view{::MapViewOfFileEx(
    file_mapping.GetHandle(), FILE_MAP_READ, 0, 0, 0, nullptr)};
  if (!mapping_view.IsValid())
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"MapViewOfFileEx failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  HookDisabler disable_create_process_hook{
    &GetDisableCreateProcessInternalWHook()};

  auto const self_dir_path = hadesmem::detail::GetSelfDirPath();
  std::wstring const helper_path =
    hadesmem::detail::CombinePath(self_dir_path, L"cerberus_helper.exe");
  auto const helper_command_line = L"\"" + helper_path + L"\" " + pid_str;
  std::vector<wchar_t> command_line_buf(std::begin(helper_command_line),
                                        std::end(helper_command_line));
  command_line_buf.push_back(L'\0');

  STARTUPINFO start_info{};
  PROCESS_INFORMATION proc_info{};
  if (!::CreateProcessW(nullptr,
                        command_line_buf.data(),
                        nullptr,
                        nullptr,
                        FALSE,
                        CREATE_NO_WINDOW,
                        nullptr,
                        nullptr,
                        &start_info,
                        &proc_info))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"CreateProcessW failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  hadesmem::detail::SmartHandle const helper_process_handle{proc_info.hProcess};
  hadesmem::detail::SmartHandle const helper_thread_handle{proc_info.hThread};

  DWORD const wait_res =
    ::WaitForSingleObject(helper_process_handle.GetHandle(), INFINITE);
  if (wait_res != WAIT_OBJECT_0)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"WaitForSingleObject failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  DWORD exit_code = 0;
  if (!::GetExitCodeProcess(helper_process_handle.GetHandle(), &exit_code))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"GetExitCodeProcess failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  DWORD session_id = 0;
  if (!::ProcessIdToSessionId(proc_info.dwProcessId, &session_id))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"ProcessIdToSessionId failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  try
  {
    if (exit_code != 0)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"Helper failed."});
    }
  }
  catch (...)
  {
    if (session_id)
    {
      throw;
    }

    HADESMEM_DETAIL_TRACE_A(
      "WARNING! Helper failed. Ignoring due to helper running in session 0.");
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
  }

  auto& render_offsets = GetRenderOffsetsImpl();
  render_offsets = *static_cast<RenderOffsets*>(mapping_view.GetHandle());
}
}
}

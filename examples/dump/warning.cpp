// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "warning.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <hadesmem/detail/filesystem.hpp>
#include <hadesmem/error.hpp>

#include "print.hpp"

namespace
{

// Record all modules (on disk) which cause a warning when dumped, to make it
// easier to isolate files which require further investigation.
// TODO: Clean this up.
bool g_warned = false;
bool g_warned_enabled = false;
bool g_warned_dynamic = false;
std::vector<std::wstring> g_all_warned;
std::wstring g_warned_file_path;
WarningType g_warned_type = WarningType::kAll;
}

void WarnForCurrentFile(WarningType warned_type)
{
  if (warned_type == g_warned_type || g_warned_type == WarningType::kAll)
  {
    g_warned = true;
  }
}

void ClearWarnForCurrentFile()
{
  g_warned = false;
}

void HandleWarnings(std::wstring const& path)
{
  if (g_warned_enabled && g_warned)
  {
    if (g_warned_dynamic)
    {
      std::unique_ptr<std::wfstream> warned_file_ptr(
        hadesmem::detail::OpenFile<wchar_t>(g_warned_file_path,
                                            std::ios::out | std::ios::app));
      std::wfstream& warned_file = *warned_file_ptr;
      if (!warned_file)
      {
        HADESMEM_DETAIL_THROW_EXCEPTION(
          hadesmem::Error()
          << hadesmem::ErrorString("Failed to open warned file for output."));
      }
      warned_file << path << '\n';
    }
    else
    {
      g_all_warned.push_back(path);
    }
  }
}

void DumpWarned(std::wostream& out)
{
  if (!g_all_warned.empty())
  {
    WriteNewline(out);
    WriteNormal(out, L"Dumping warned list.", 0);
    for (auto const f : g_all_warned)
    {
      WriteNormal(out, f, 0);
    }
  }
}

bool GetWarningsEnabled()
{
  return g_warned_enabled;
}

void SetWarningsEnabled(bool b)
{
  g_warned_enabled = b;
}

bool GetDynamicWarningsEnabled()
{
  return g_warned_dynamic;
}

void SetDynamicWarningsEnabled(bool b)
{
  g_warned_dynamic = b;
}

std::wstring GetWarnedFilePath()
{
  return g_warned_file_path;
}

void SetWarnedFilePath(std::wstring const& path)
{
  g_warned_file_path = path;
}

WarningType GetWarnedType()
{
  return g_warned_type;
}

void SetWarnedType(WarningType warned_type)
{
  g_warned_type = warned_type;
}

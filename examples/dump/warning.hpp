// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <iosfwd>
#include <string>

enum class WarningType : int
{
  kSuspicious,
  kUnsupported,
  kAll = -1
};

void WarnForCurrentFile(WarningType warned_type);

void ClearWarnForCurrentFile();

void HandleWarnings(std::wstring const& path);

void DumpWarned(std::wostream& out);

bool GetWarningsEnabled();

void SetWarningsEnabled(bool b);

bool GetDynamicWarningsEnabled();

void SetDynamicWarningsEnabled(bool b);

std::wstring GetWarnedFilePath();

void SetWarnedFilePath(std::wstring const& path);

WarningType GetWarnedType();

void SetWarnedType(WarningType warned_type);

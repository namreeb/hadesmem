// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <utility>

#include <windows.h>

#include "hadesmem/config.hpp"

namespace hadesmem
{

class Process;

template <typename T>
class CallResult;

struct InjectFlags
{
  enum
  {
    kNone = 0, 
    kPathResolution = 1 << 0, 
    kAddToSearchOrder = 1 << 1, 
    kKeepSuspended = 1 << 2, 
    kInvalidFlagMaxValue = 1 << 3
  };
};

HMODULE InjectDll(Process const& process, std::wstring const& path, 
  int flags);

void FreeDll(Process const& process, HMODULE module);

CallResult<DWORD_PTR> CallExport(Process const& process, HMODULE module, 
  std::string const& export_name);

class CreateAndInjectData
{
public:
  explicit CreateAndInjectData(Process const& process, HMODULE module, 
    DWORD_PTR export_ret, DWORD export_last_error);

  CreateAndInjectData(CreateAndInjectData const& other);

  CreateAndInjectData& operator=(CreateAndInjectData const& other);

  CreateAndInjectData(CreateAndInjectData&& other) HADESMEM_NOEXCEPT;

  CreateAndInjectData& operator=(CreateAndInjectData&& other) 
    HADESMEM_NOEXCEPT;

  ~CreateAndInjectData();

  Process GetProcess() const;

  HMODULE GetModule() const HADESMEM_NOEXCEPT;

  DWORD_PTR GetExportRet() const HADESMEM_NOEXCEPT;

  DWORD GetExportLastError() const HADESMEM_NOEXCEPT;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

CreateAndInjectData CreateAndInject(
  std::wstring const& path, 
  std::wstring const& work_dir, 
  std::vector<std::wstring> const& args, 
  std::wstring const& module, 
  std::string const& export_name, 
  int flags);

}

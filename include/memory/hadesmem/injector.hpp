// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <string>
#include <vector>
#include <utility>

#include <windows.h>

#include "hadesmem/call.hpp"
#include "hadesmem/config.hpp"
#include "hadesmem/process.hpp"

namespace hadesmem
{

class Process;

struct InjectFlags
{
  enum
  {
    kNone = 0, 
    kPathResolution = 1 << 0, 
    kAddToSearchOrder = 1 << 1, 
    kInvalidFlagMaxValue = 1 << 2
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
  CreateAndInjectData(Process const& process, HMODULE module, 
    DWORD_PTR export_ret, DWORD export_last_error);

  CreateAndInjectData(CreateAndInjectData const& Other);

  CreateAndInjectData& operator=(CreateAndInjectData const& Other);

  CreateAndInjectData(CreateAndInjectData&& Other) HADESMEM_NOEXCEPT;

  CreateAndInjectData& operator=(CreateAndInjectData&& Other) HADESMEM_NOEXCEPT;

  Process GetProcess() const;

  HMODULE GetModule() const HADESMEM_NOEXCEPT;

  DWORD_PTR GetExportRet() const HADESMEM_NOEXCEPT;

  DWORD GetExportLastError() const HADESMEM_NOEXCEPT;

private:
  Process process_;
  HMODULE module_;
  DWORD_PTR export_ret_;
  DWORD export_last_error_;
};

CreateAndInjectData CreateAndInject(
  std::wstring const& path, 
  std::wstring const& work_dir, 
  std::vector<std::wstring> const& args, 
  std::wstring const& module, 
  std::string const& export_name, 
  int flags);

}

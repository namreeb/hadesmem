// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <string>
#include <vector>
#include <utility>

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/process.hpp>

namespace hadesmem
{

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
    DWORD_PTR export_ret, DWORD export_last_error)
    : process_(process), 
    module_(module), 
    export_ret_(export_ret), 
    export_last_error_(export_last_error)
  { }

  CreateAndInjectData(CreateAndInjectData const& other)
    : process_(other.process_), 
    module_(other.module_), 
    export_ret_(other.export_ret_), 
    export_last_error_(other.export_last_error_)
  { }

  CreateAndInjectData& operator=(CreateAndInjectData const& other)
  {
    process_ = other.process_;
    module_ = other.module_;
    export_ret_ = other.export_ret_;
    export_last_error_ = other.export_last_error_;

    return *this;
  }

  CreateAndInjectData(CreateAndInjectData&& other) HADESMEM_NOEXCEPT
    : process_(std::move(other.process_)), 
    module_(other.module_), 
    export_ret_(other.export_ret_), 
    export_last_error_(other.export_last_error_)
  { }

  CreateAndInjectData& operator=(CreateAndInjectData&& other) 
    HADESMEM_NOEXCEPT
  {
    process_ = std::move(other.process_);
    module_ = other.module_;
    export_ret_ = other.export_ret_;
    export_last_error_ = other.export_last_error_;

    return *this;
  }

  ~CreateAndInjectData() HADESMEM_NOEXCEPT
  { }

  Process GetProcess() const
  {
    return process_;
  }

  HMODULE GetModule() const HADESMEM_NOEXCEPT
  {
    return module_;
  }

  DWORD_PTR GetExportRet() const HADESMEM_NOEXCEPT
  {
    return export_ret_;
  }

  DWORD GetExportLastError() const HADESMEM_NOEXCEPT
  {
    return export_last_error_;
  }

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

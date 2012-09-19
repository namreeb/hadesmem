// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/injector.hpp"

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/filesystem.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/call.hpp"
#include "hadesmem/alloc.hpp"
#include "hadesmem/error.hpp"
#include "hadesmem/write.hpp"
#include "hadesmem/module.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/detail/self_path.hpp"

namespace hadesmem
{

HMODULE InjectDll(Process const& process, std::wstring const& path, 
  InjectFlags flags)
{
  BOOST_ASSERT((static_cast<int>(flags) & 
    ~(static_cast<int>(InjectFlags::InvalidFlagMaxValue) - 1)) == 0);

  // Do not continue if Shim Engine is enabled for local process, 
  // otherwise it could interfere with the address resolution.
  HMODULE const shim_eng_mod = GetModuleHandle(L"ShimEng.dll");
  if (shim_eng_mod)
  {
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Shims enabled for local process."));
  }

  boost::filesystem::path path_real(path);

  bool const path_resolution = 
    ((static_cast<int>(flags) & static_cast<int>(InjectFlags::PathResolution)) 
    == static_cast<int>(InjectFlags::PathResolution));

  if (path_resolution && path_real.is_relative())
  {
    path_real = boost::filesystem::absolute(path_real, 
      detail::GetSelfDirPath());
  }

  path_real.make_preferred();

  // Ensure target file exists
  // Note: Only performing this check when path resolution is enabled, 
  // because otherwise we would need to perform the check in the context 
  // of the remote process, which is not possible to do without 
  // introducing race conditions and other potential problems. So we just 
  // let LoadLibraryW do the check for us.
  if (path_resolution && !boost::filesystem::exists(path_real))
  {
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Could not find module file."));
  }

  std::wstring const path_string(path_real.native());
  std::size_t const path_buf_size = (path_string.size() + 1) * 
    sizeof(wchar_t);

  Allocator const lib_file_remote(&process, path_buf_size);
  WriteString(process, lib_file_remote.GetBase(), path_string);

  Module const kernel32_mod(&process, L"kernel32.dll");
  LPCVOID load_library = reinterpret_cast<LPCVOID>(reinterpret_cast<DWORD_PTR>(
    FindProcedure(kernel32_mod, "LoadLibraryW")));

  typedef HMODULE (*LoadLibraryFuncT)(LPCWSTR lpFileName);
  std::pair<HMODULE, DWORD> const load_library_ret = 
    Call<LoadLibraryFuncT>(process, load_library, CallConv::kWinApi, 
    static_cast<LPCWSTR>(lib_file_remote.GetBase()));
  if (!load_library_ret.first)
  {
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Call to LoadLibraryW in remote process failed.") << 
      ErrorCodeWinLast(load_library_ret.second));
  }

  return load_library_ret.first;
}

void FreeDll(Process const& process, HMODULE module)
{
  Module const kernel32_mod(&process, L"kernel32.dll");
  LPCVOID free_library = reinterpret_cast<LPCVOID>(reinterpret_cast<DWORD_PTR>(
    FindProcedure(kernel32_mod, "FreeLibrary")));

  typedef BOOL (*FreeLibraryFuncT)(HMODULE hModule);
  std::pair<BOOL, DWORD> const free_library_ret = 
    Call<FreeLibraryFuncT>(process, free_library, CallConv::kWinApi, module);
  if (!free_library_ret.first)
  {
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Call to FreeLibrary in remote process failed.") << 
      ErrorCodeWinLast(free_library_ret.second));
  }
}


}

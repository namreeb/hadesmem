// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <iosfwd>
#include <memory>
#include <string>
#include <cstddef>
#include <ostream>
#include <utility>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/assert.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <windows.h>
#include <winnt.h>

#include <hadesmem/read.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/write.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/pelib/nt_headers.hpp>

// TODO: Fix the code so this hack can be removed.
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextended-offsetof"
#endif

namespace hadesmem
{

class ImportDir
{
public:
  explicit ImportDir(Process const& process, PeFile const& pe_file, 
    PIMAGE_IMPORT_DESCRIPTOR imp_desc)
    : process_(&process), 
    pe_file_(&pe_file), 
    base_(reinterpret_cast<PBYTE>(imp_desc))
  {
    if (!base_)
    {
      NtHeaders nt_headers(process, pe_file);
      DWORD const import_dir_rva = nt_headers.GetDataDirectoryVirtualAddress(
        PeDataDir::Import);
      // Windows will load images which don't specify a size for the import 
      // directory.
      if (!import_dir_rva)
      {
        HADESMEM_THROW_EXCEPTION(Error() << 
          ErrorString("Import directory is invalid."));
      }

      base_ = static_cast<PBYTE>(RvaToVa(process, pe_file, import_dir_rva));
    }
  }

  ImportDir(ImportDir const& other) HADESMEM_NOEXCEPT
    : process_(other.process_), 
    pe_file_(other.pe_file_), 
    base_(other.base_)
  { }
  
  ImportDir& operator=(ImportDir const& other) HADESMEM_NOEXCEPT
  {
    process_ = other.process_;
    pe_file_ = other.pe_file_;
    base_ = other.base_;

    return *this;
  }

  ImportDir(ImportDir&& other) HADESMEM_NOEXCEPT
    : process_(other.process_), 
    pe_file_(other.pe_file_), 
    base_(other.base_)
  { }
  
  ImportDir& operator=(ImportDir&& other) HADESMEM_NOEXCEPT
  {
    process_ = other.process_;
    pe_file_ = other.pe_file_;
    base_ = other.base_;

    return *this;
  }
  
  ~ImportDir() HADESMEM_NOEXCEPT
  { }

  PVOID GetBase() const HADESMEM_NOEXCEPT
  {
    return base_;
  }
  
  DWORD GetOriginalFirstThunk() const
  {
    return Read<DWORD>(*process_, base_ +  offsetof(IMAGE_IMPORT_DESCRIPTOR, 
      OriginalFirstThunk));
  }

  DWORD GetTimeDateStamp() const
  {
    return Read<DWORD>(*process_, base_ +  offsetof(IMAGE_IMPORT_DESCRIPTOR, 
      TimeDateStamp));
  }

  DWORD GetForwarderChain() const
  {
    return Read<DWORD>(*process_, base_ +  offsetof(IMAGE_IMPORT_DESCRIPTOR, 
      ForwarderChain));
  }

  DWORD GetNameRaw() const
  {
    return Read<DWORD>(*process_, base_ +  offsetof(IMAGE_IMPORT_DESCRIPTOR, 
      Name));
  }

  std::string GetName() const
  {
    return ReadString<char>(*process_, RvaToVa(*process_, *pe_file_, 
      GetNameRaw()));
  }

  DWORD GetFirstThunk() const
  {
    return Read<DWORD>(*process_, base_ +  offsetof(IMAGE_IMPORT_DESCRIPTOR, 
      FirstThunk));
  }

  void SetOriginalFirstThunk(DWORD original_first_thunk)
  {
    return Write(*process_, base_ +  offsetof(IMAGE_IMPORT_DESCRIPTOR, 
      OriginalFirstThunk), original_first_thunk);
  }

  void SetTimeDateStamp(DWORD time_date_stamp)
  {
    return Write(*process_, base_ +  offsetof(IMAGE_IMPORT_DESCRIPTOR, 
      TimeDateStamp), time_date_stamp);
  }

  void SetForwarderChain(DWORD forwarder_chain)
  {
    return Write(*process_, base_ +  offsetof(IMAGE_IMPORT_DESCRIPTOR, 
      ForwarderChain), forwarder_chain);
  }

  void SetNameRaw(DWORD name)
  {
    Write(*process_, base_ +  offsetof(
      IMAGE_IMPORT_DESCRIPTOR, Name), name);
  }

  void SetName(std::string const& name)
  {
    DWORD name_rva = Read<DWORD>(*process_, base_ +  offsetof(
      IMAGE_IMPORT_DESCRIPTOR, Name));
    if (!name_rva)
    {
      HADESMEM_THROW_EXCEPTION(Error() << 
        ErrorString("Name RVA is null."));
    }

    PVOID name_ptr = RvaToVa(*process_, *pe_file_, name_rva);
    if (!name_ptr)
    {
      HADESMEM_THROW_EXCEPTION(Error() << 
        ErrorString("Name VA is null."));
    }

    std::string const cur_name = ReadString<char>(*process_, name_ptr);

    // TODO: Support allocating space for a new name rather than just 
    // overwriting the existing one.
    if (name.size() > cur_name.size())
    {
      HADESMEM_THROW_EXCEPTION(Error() << 
        ErrorString("New name longer than existing name."));
    }

    return WriteString(*process_, name_ptr, name);
  }

  void SetFirstThunk(DWORD first_thunk)
  {
    return Write(*process_, base_ +  offsetof(IMAGE_IMPORT_DESCRIPTOR, 
      FirstThunk), first_thunk);
  }

private:
  Process const* process_;
  PeFile const* pe_file_;
  PBYTE base_;
};

inline bool operator==(ImportDir const& lhs, ImportDir const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

inline bool operator!=(ImportDir const& lhs, ImportDir const& rhs) HADESMEM_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(ImportDir const& lhs, ImportDir const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

inline bool operator<=(ImportDir const& lhs, ImportDir const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

inline bool operator>(ImportDir const& lhs, ImportDir const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

inline bool operator>=(ImportDir const& lhs, ImportDir const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

inline std::ostream& operator<<(std::ostream& lhs, ImportDir const& rhs)
{
  return (lhs << rhs.GetBase());
}

inline std::wostream& operator<<(std::wostream& lhs, ImportDir const& rhs)
{
  return (lhs << rhs.GetBase());
}

}

#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic pop
#endif

// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstddef>
#include <iosfwd>
#include <locale>
#include <memory>
#include <ostream>
#include <string>
#include <utility>

#include <windows.h>
#include <winnt.h>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/pelib/bound_import_fwd_ref.hpp>
#include <hadesmem/pelib/nt_headers.hpp>
#include <hadesmem/pelib/import_dir.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

// TODO: Add tests.

namespace hadesmem
{

class BoundImportDescriptor
{
public:
  explicit BoundImportDescriptor(Process const& process,
                                 PeFile const& pe_file,
                                 PIMAGE_BOUND_IMPORT_DESCRIPTOR start,
                                 PIMAGE_BOUND_IMPORT_DESCRIPTOR imp_desc)
    : process_(&process),
      pe_file_(&pe_file),
      start_(reinterpret_cast<PBYTE>(start)),
      base_(reinterpret_cast<PBYTE>(imp_desc)),
      data_()
  {
    HADESMEM_DETAIL_ASSERT((start_ && base_) || (!start_ && !base_));

    if (!base_)
    {
      NtHeaders nt_headers(process, pe_file);
      DWORD const import_dir_rva =
        nt_headers.GetDataDirectoryVirtualAddress(PeDataDir::BoundImport);
      // Windows will load images which don't specify a size for the import
      // directory.
      if (!import_dir_rva)
      {
        HADESMEM_DETAIL_THROW_EXCEPTION(
          Error() << ErrorString("Bound import directory is invalid."));
      }

      base_ = static_cast<PBYTE>(RvaToVa(process, pe_file, import_dir_rva));
      if (!base_)
      {
        HADESMEM_DETAIL_THROW_EXCEPTION(
          Error() << ErrorString("Bound import directory is invalid."));
      }
    }

    if (!start_)
    {
      start_ = base_;
    }

    UpdateRead();
  }

  PVOID GetBase() const HADESMEM_DETAIL_NOEXCEPT
  {
    return base_;
  }

  PVOID GetStart() const HADESMEM_DETAIL_NOEXCEPT
  {
    return start_;
  }

  void UpdateRead()
  {
    data_ = Read<IMAGE_BOUND_IMPORT_DESCRIPTOR>(*process_, base_);
  }

  void UpdateWrite()
  {
    Write(*process_, base_, data_);
  }

  DWORD GetTimeDateStamp() const
  {
    return data_.TimeDateStamp;
  }

  WORD GetOffsetModuleName() const
  {
    return data_.OffsetModuleName;
  }

  std::string GetModuleName() const
  {
    return detail::CheckedReadString<char>(
      *process_, *pe_file_, start_ + GetOffsetModuleName());
  }

  WORD GetNumberOfModuleForwarderRefs() const
  {
    return data_.NumberOfModuleForwarderRefs;
  }

  void SetTimeDateStamp(DWORD time_date_stamp)
  {
    data_.TimeDateStamp = time_date_stamp;
  }

  void SetOffsetModuleName(WORD offset_module_name)
  {
    data_.OffsetModuleName = offset_module_name;
  }

  // TODO: SetModuleName

  void SetNumberOfModuleForwarderRefs(WORD number_of_module_forwarder_refs)
  {
    data_.NumberOfModuleForwarderRefs = number_of_module_forwarder_refs;
  }

  // TODO: SetModuleForwarderRefs

private:
  Process const* process_;
  PeFile const* pe_file_;
  PBYTE start_;
  PBYTE base_;
  IMAGE_BOUND_IMPORT_DESCRIPTOR data_;
};

inline bool operator==(BoundImportDescriptor const& lhs,
                       BoundImportDescriptor const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

inline bool operator!=(BoundImportDescriptor const& lhs,
                       BoundImportDescriptor const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(BoundImportDescriptor const& lhs,
                      BoundImportDescriptor const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

inline bool operator<=(BoundImportDescriptor const& lhs,
                       BoundImportDescriptor const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

inline bool operator>(BoundImportDescriptor const& lhs,
                      BoundImportDescriptor const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

inline bool operator>=(BoundImportDescriptor const& lhs,
                       BoundImportDescriptor const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

inline std::ostream& operator<<(std::ostream& lhs,
                                BoundImportDescriptor const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << rhs.GetBase();
  lhs.imbue(old);
  return lhs;
}

inline std::wostream& operator<<(std::wostream& lhs,
                                 BoundImportDescriptor const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << rhs.GetBase();
  lhs.imbue(old);
  return lhs;
}
}

// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstddef>
#include <iosfwd>
#include <locale>
#include <ostream>
#include <string>

#include <windows.h>
#include <winnt.h>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

// TODO: Add tests.

namespace hadesmem
{

class BoundImportForwarderRef
{
public:
  explicit BoundImportForwarderRef(Process const& process,
                                   PeFile const& pe_file,
                                   PIMAGE_BOUND_IMPORT_DESCRIPTOR start,
                                   PIMAGE_BOUND_FORWARDER_REF fwd_ref)
    : process_(&process),
      pe_file_(&pe_file),
      start_(reinterpret_cast<PBYTE>(start)),
      base_(reinterpret_cast<PBYTE>(fwd_ref)),
      data_()
  {
    HADESMEM_DETAIL_ASSERT(start_ && base_);

    UpdateRead();
  }

  PVOID GetBase() const HADESMEM_DETAIL_NOEXCEPT
  {
    return base_;
  }

  void UpdateRead()
  {
    data_ = Read<IMAGE_BOUND_FORWARDER_REF>(*process_, base_);
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
    // OffsetModuleName should never be zero, but apparently it's possible to
    // have some files where it is zero anyway... Probably because the timestamp
    // is intentionally invalid so it's never matched. For now, just ignore
    // this case and hope for the best.
    // TODO: Fix this parsing of files like this properly.
    return detail::CheckedReadString<char>(
      *process_, *pe_file_, start_ + GetOffsetModuleName());
  }

  WORD GetReserved() const
  {
    return data_.Reserved;
  }

  void SetTimeDateStamp(DWORD time_date_stamp)
  {
    data_.TimeDateStamp = time_date_stamp;
  }

  void SetOffsetModuleName(WORD offset_module_name)
  {
    data_.OffsetModuleName = offset_module_name;
  }

  void SetReserved(WORD reserved)
  {
    data_.Reserved = reserved;
  }

  // TODO: SetModuleName

private:
  Process const* process_;
  PeFile const* pe_file_;
  PBYTE start_;
  PBYTE base_;
  IMAGE_BOUND_FORWARDER_REF data_;
};

inline bool operator==(BoundImportForwarderRef const& lhs,
                       BoundImportForwarderRef const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

inline bool operator!=(BoundImportForwarderRef const& lhs,
                       BoundImportForwarderRef const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(BoundImportForwarderRef const& lhs,
                      BoundImportForwarderRef const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

inline bool operator<=(BoundImportForwarderRef const& lhs,
                       BoundImportForwarderRef const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

inline bool operator>(BoundImportForwarderRef const& lhs,
                      BoundImportForwarderRef const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

inline bool operator>=(BoundImportForwarderRef const& lhs,
                       BoundImportForwarderRef const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

inline std::ostream& operator<<(std::ostream& lhs,
                                BoundImportForwarderRef const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << rhs.GetBase();
  lhs.imbue(old);
  return lhs;
}

inline std::wostream& operator<<(std::wostream& lhs,
                                 BoundImportForwarderRef const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << rhs.GetBase();
  lhs.imbue(old);
  return lhs;
}
}

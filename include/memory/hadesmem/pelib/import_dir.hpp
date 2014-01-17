// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstddef>
#include <iosfwd>
#include <memory>
#include <ostream>
#include <string>
#include <utility>

#include <windows.h>
#include <winnt.h>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/pelib/nt_headers.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/pelib/tls_dir.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

// TODO: Handle forwarded imports.

// TODO: Handle bound imports (both new way and old way).

// TODO: This should really be called ImportDescriptor.

namespace hadesmem
{

class ImportDir
{
public:
  explicit ImportDir(Process const& process,
                     PeFile const& pe_file,
                     PIMAGE_IMPORT_DESCRIPTOR imp_desc)
    : process_(&process),
      pe_file_(&pe_file),
      base_(reinterpret_cast<PBYTE>(imp_desc)),
      data_(), 
      is_virtual_beg_(false)
  {
    if (!base_)
    {
      NtHeaders nt_headers(process, pe_file);
      DWORD const import_dir_rva =
        nt_headers.GetDataDirectoryVirtualAddress(PeDataDir::Import);
      // Windows will load images which don't specify a size for the import
      // directory.
      if (!import_dir_rva)
      {
        HADESMEM_DETAIL_THROW_EXCEPTION(
          Error() << ErrorString("Import directory is invalid."));
      }

      base_ = static_cast<PBYTE>(RvaToVa(process, pe_file, import_dir_rva));
      if (!base_)
      {
        // Try to detect import dirs with a partially virtual descriptor
        // (overlapped at the beginning). Up to the first 3 DWORDS can be
        // overlapped (because they're allowed to be zero without invalidating
        // the entry).
        // Sample: imports_virtdesc.exe (Corkami PE Corpus)
        void* desc_raw_beg = nullptr;
        int i = 3;
        do
        {
          auto const new_rva =
            static_cast<DWORD>(import_dir_rva + sizeof(DWORD) * i);
          auto const new_va = RvaToVa(process, pe_file, new_rva);
          if (!new_va)
          {
            break;
          }
          desc_raw_beg = new_va;
        } while (--i);

        if (desc_raw_beg)
        {
          auto const offset = sizeof(DWORD) * (i + 1);
          auto const len = sizeof(IMAGE_IMPORT_DESCRIPTOR) - offset;
          auto const buf =
            ReadVector<std::uint8_t>(*process_, desc_raw_beg, len);
          auto const data_beg =
            reinterpret_cast<std::uint8_t*>(&data_) + offset;
          ZeroMemory(&data_, sizeof(data_));
          std::copy(std::begin(buf), std::end(buf), data_beg);
          base_ = static_cast<std::uint8_t*>(desc_raw_beg) - offset;
          is_virtual_beg_ = true;
        }
        else
        {
          HADESMEM_DETAIL_THROW_EXCEPTION(
            Error() << ErrorString("Import directory is invalid."));
        }
      }
    }

    UpdateRead();
  }

  PVOID GetBase() const HADESMEM_DETAIL_NOEXCEPT
  {
    return base_;
  }

  void UpdateRead()
  {
    data_ = Read<IMAGE_IMPORT_DESCRIPTOR>(*process_, base_);
  }

  void UpdateWrite()
  {
    Write(*process_, base_, data_);
  }

  // Check for virtual descriptor overlap trick.
  bool IsVirtualBegin() const
  {
    return is_virtual_beg_;
  }

  // Check for virtual termination trick.
  // TODO: Think about what the best way to solve this is... Currently we're
  // forcing the user to thunk about it, which may not be ideal.
  bool IsVirtualTerminated() const
  {
    // It's possible for the last entry to be in virtual space, because it only
    // needs to have its Name or FirstThunk null.
    // Sample: imports_vterm.exe (Corkami PE Corpus)
    // TODO: Fix this for cases where a virtual descriptor is 'real', rather
    // than just used as a terminator.
    if (pe_file_->GetType() == PeFileType::Data &&
        (base_ + sizeof(IMAGE_IMPORT_DESCRIPTOR)) >=
          static_cast<PBYTE>(pe_file_->GetBase()) + pe_file_->GetSize())
    {
      return true;
    }

    return false;
  }

  // Check for TLS AOI trick.
  // Sample: manyimportsW7.exe (Corkami PE Corpus)
  // TODO: Think about what the best way to solve this is... Currently we're
  // forcing the user to thunk about it, which may not be ideal.
  bool IsTlsAoiTerminated() const
  {
    try
    {
      TlsDir tls_dir(*process_, *pe_file_);
      ULONG_PTR const image_base = GetRuntimeBase(*process_, *pe_file_);
      auto const address_of_index_raw =
        RvaToVa(*process_,
                *pe_file_,
                static_cast<DWORD>(tls_dir.GetAddressOfIndex() - image_base));
      return (address_of_index_raw ==
                base_ + offsetof(IMAGE_IMPORT_DESCRIPTOR, Name) ||
              address_of_index_raw ==
                base_ + offsetof(IMAGE_IMPORT_DESCRIPTOR, FirstThunk));
    }
    catch (std::exception const& /*e*/)
    {
      return false;
    }
  }

  DWORD GetOriginalFirstThunk() const
  {
    return data_.OriginalFirstThunk;
  }

  DWORD GetTimeDateStamp() const
  {
    return data_.TimeDateStamp;
  }

  DWORD GetForwarderChain() const
  {
    return data_.ForwarderChain;
  }

  DWORD GetNameRaw() const
  {
    return data_.Name;
  }

  std::string GetName() const
  {
    DWORD const name_rva = GetNameRaw();
    if (!name_rva)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                      << ErrorString("Name RVA is invalid."));
    }

    auto name_va =
      static_cast<std::uint8_t*>(RvaToVa(*process_, *pe_file_, name_rva));
    // It's possible for the RVA to be invalid on disk because it's fixed by
    // relocations.
    // TODO: Handle this case.
    // Sample: imports_relocW7.exe
    if (!name_va)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                      << ErrorString("Name VA is invalid."));
    }

    if (pe_file_->GetType() == PeFileType::Image)
    {
      return ReadString<char>(*process_, name_va);
    }
    else if (pe_file_->GetType() == PeFileType::Data)
    {
      std::string name;
      // Handle EOF termination.
      // Sample: maxsecXP.exe (Corkami PE Corpus)
      // TODO: Fix the perf of this.
      // TODO: Detect and handle the case where the string is terminated
      // virtually.
      void const* const file_end =
        static_cast<std::uint8_t const*>(pe_file_->GetBase()) +
        pe_file_->GetSize();
      while (name_va < file_end)
      {
        if (char const c = Read<char>(*process_, name_va++))
        {
          name.push_back(c);
        }
        else
        {
          break;
        }
      }
      return name;
    }
    else
    {
      HADESMEM_DETAIL_ASSERT(false);
      HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                      << ErrorString("Unknown PE file type."));
    }
  }

  DWORD GetFirstThunk() const
  {
    return data_.FirstThunk;
  }

  void SetOriginalFirstThunk(DWORD original_first_thunk)
  {
    data_.OriginalFirstThunk = original_first_thunk;
  }

  void SetTimeDateStamp(DWORD time_date_stamp)
  {
    data_.TimeDateStamp = time_date_stamp;
  }

  void SetForwarderChain(DWORD forwarder_chain)
  {
    data_.ForwarderChain = forwarder_chain;
  }

  void SetNameRaw(DWORD name)
  {
    data_.Name = name;
  }

  void SetName(std::string const& name)
  {
    DWORD name_rva =
      Read<DWORD>(*process_, base_ + offsetof(IMAGE_IMPORT_DESCRIPTOR, Name));
    if (!name_rva)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                      << ErrorString("Name RVA is null."));
    }

    PVOID name_ptr = RvaToVa(*process_, *pe_file_, name_rva);
    if (!name_ptr)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                      << ErrorString("Name VA is null."));
    }

    std::string const cur_name = ReadString<char>(*process_, name_ptr);

    // TODO: Support allocating space for a new name rather than just
    // overwriting the existing one.
    if (name.size() > cur_name.size())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error() << ErrorString("New name longer than existing name."));
    }

    return WriteString(*process_, name_ptr, name);
  }

  void SetFirstThunk(DWORD first_thunk)
  {
    data_.FirstThunk = first_thunk;
  }

private:
  Process const* process_;
  PeFile const* pe_file_;
  PBYTE base_;
  IMAGE_IMPORT_DESCRIPTOR data_;
  bool is_virtual_beg_;
};

inline bool operator==(ImportDir const& lhs, ImportDir const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

inline bool operator!=(ImportDir const& lhs, ImportDir const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(ImportDir const& lhs, ImportDir const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

inline bool operator<=(ImportDir const& lhs, ImportDir const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

inline bool operator>(ImportDir const& lhs, ImportDir const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

inline bool operator>=(ImportDir const& lhs, ImportDir const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

inline std::ostream& operator<<(std::ostream& lhs, ImportDir const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << rhs.GetBase();
  lhs.imbue(old);
  return lhs;
}

inline std::wostream& operator<<(std::wostream& lhs, ImportDir const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << rhs.GetBase();
  lhs.imbue(old);
  return lhs;
}
}

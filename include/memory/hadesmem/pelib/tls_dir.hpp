// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <ostream>
#include <vector>
#include <utility>

#include <windows.h>
#include <winnt.h>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/pelib/nt_headers.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

namespace hadesmem
{
class TlsDir
{
public:
  explicit TlsDir(Process const& process, PeFile const& pe_file)
    : process_{&process}, pe_file_{&pe_file}
  {
    NtHeaders const nt_headers{process, pe_file};

    DWORD const data_dir_va =
      nt_headers.GetDataDirectoryVirtualAddress(PeDataDir::TLS);
    // Windows will load images which don't specify a size for the
    // TLS directory.
    if (!data_dir_va)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error{} << ErrorString{"PE file has no TLS directory."});
    }

    base_ = static_cast<std::uint8_t*>(RvaToVa(process, pe_file, data_dir_va));
    if (!base_)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error{} << ErrorString{"TLS directory is invalid."});
    }

    UpdateRead();
  }

  void* GetBase() const noexcept
  {
    return base_;
  }

  void UpdateRead()
  {
    if (pe_file_->Is64())
    {
      data_64_ = Read<IMAGE_TLS_DIRECTORY64>(*process_, base_);
    }
    else
    {
      data_32_ = Read<IMAGE_TLS_DIRECTORY32>(*process_, base_);
    }
  }

  void UpdateWrite()
  {
    if (pe_file_->Is64())
    {
      Write(*process_, base_, data_64_);
    }
    else
    {
      Write(*process_, base_, data_32_);
    }
  }

  ULONGLONG GetStartAddressOfRawData() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.StartAddressOfRawData;
    }
    else
    {
      return data_32_.StartAddressOfRawData;
    }
  }

  ULONGLONG GetEndAddressOfRawData() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.EndAddressOfRawData;
    }
    else
    {
      return data_32_.EndAddressOfRawData;
    }
  }

  ULONGLONG GetAddressOfIndex() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.AddressOfIndex;
    }
    else
    {
      return data_32_.AddressOfIndex;
    }
  }

  ULONGLONG GetAddressOfCallBacks() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.AddressOfCallBacks;
    }
    else
    {
      return data_32_.AddressOfCallBacks;
    }
  }

  template <typename OutputIterator>
  void GetCallbacks(OutputIterator callbacks) const
  {
    using OutputIteratorCategory =
      typename std::iterator_traits<OutputIterator>::iterator_category;
    HADESMEM_DETAIL_STATIC_ASSERT(
      std::is_base_of<std::output_iterator_tag, OutputIteratorCategory>::value);

    pe_file_->Is64() ? GetCallbacksImpl<ULONGLONG>(callbacks)
                     : GetCallbacksImpl<DWORD>(callbacks);
  }

  // TODO: Add SetCallbacks.

  DWORD GetSizeOfZeroFill() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.SizeOfZeroFill;
    }
    else
    {
      return data_32_.SizeOfZeroFill;
    }
  }

  DWORD GetCharacteristics() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.Characteristics;
    }
    else
    {
      return data_32_.Characteristics;
    }
  }

  void SetStartAddressOfRawData(ULONGLONG start_address_of_raw_data)
  {
    if (pe_file_->Is64())
    {
      data_64_.StartAddressOfRawData = start_address_of_raw_data;
    }
    else
    {
      data_32_.StartAddressOfRawData =
        static_cast<DWORD>(start_address_of_raw_data);
    }
  }

  void SetEndAddressOfRawData(ULONGLONG end_address_of_raw_data)
  {
    if (pe_file_->Is64())
    {
      data_64_.EndAddressOfRawData = end_address_of_raw_data;
    }
    else
    {
      data_32_.EndAddressOfRawData =
        static_cast<DWORD>(end_address_of_raw_data);
    }
  }

  void SetAddressOfIndex(ULONGLONG address_of_index)
  {
    if (pe_file_->Is64())
    {
      data_64_.AddressOfIndex = address_of_index;
    }
    else
    {
      data_32_.AddressOfIndex = static_cast<DWORD>(address_of_index);
    }
  }

  void SetAddressOfCallBacks(ULONGLONG address_of_callbacks)
  {
    if (pe_file_->Is64())
    {
      data_64_.AddressOfCallBacks = address_of_callbacks;
    }
    else
    {
      data_32_.AddressOfCallBacks = static_cast<DWORD>(address_of_callbacks);
    }
  }

  void SetSizeOfZeroFill(DWORD size_of_zero_fill)
  {
    if (pe_file_->Is64())
    {
      data_64_.SizeOfZeroFill = size_of_zero_fill;
    }
    else
    {
      data_32_.SizeOfZeroFill = size_of_zero_fill;
    }
  }

  void SetCharacteristics(DWORD characteristics)
  {
    if (pe_file_->Is64())
    {
      data_64_.Characteristics = characteristics;
    }
    else
    {
      data_32_.Characteristics = characteristics;
    }
  }

private:
  template <typename T, typename OutputIterator>
  void GetCallbacksImpl(OutputIterator callbacks) const
  {
    auto const image_base = GetRuntimeBase(*process_, *pe_file_);
    auto callbacks_raw = reinterpret_cast<T*>(
      RvaToVa(*process_,
              *pe_file_,
              static_cast<DWORD>(GetAddressOfCallBacks() - image_base)));
    if (!callbacks_raw)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error{} << ErrorString{"TLS callbacks are invalid."});
    }

    for (auto callback = Read<T>(*process_, callbacks_raw); callback;
         callback = Read<T>(*process_, ++callbacks_raw))
    {
      *callbacks++ = static_cast<ULONGLONG>(callback) - image_base;
    }
  }

  Process const* process_;
  PeFile const* pe_file_;
  std::uint8_t* base_{};
  IMAGE_TLS_DIRECTORY32 data_32_ = IMAGE_TLS_DIRECTORY32{};
  IMAGE_TLS_DIRECTORY64 data_64_ = IMAGE_TLS_DIRECTORY64{};
};

inline bool operator==(TlsDir const& lhs, TlsDir const& rhs) noexcept
{
  return lhs.GetBase() == rhs.GetBase();
}

inline bool operator!=(TlsDir const& lhs, TlsDir const& rhs) noexcept
{
  return !(lhs == rhs);
}

inline bool operator<(TlsDir const& lhs, TlsDir const& rhs) noexcept
{
  return lhs.GetBase() < rhs.GetBase();
}

inline bool operator<=(TlsDir const& lhs, TlsDir const& rhs) noexcept
{
  return lhs.GetBase() <= rhs.GetBase();
}

inline bool operator>(TlsDir const& lhs, TlsDir const& rhs) noexcept
{
  return lhs.GetBase() > rhs.GetBase();
}

inline bool operator>=(TlsDir const& lhs, TlsDir const& rhs) noexcept
{
  return lhs.GetBase() >= rhs.GetBase();
}

inline std::ostream& operator<<(std::ostream& lhs, TlsDir const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << rhs.GetBase();
  lhs.imbue(old);
  return lhs;
}

inline std::wostream& operator<<(std::wostream& lhs, TlsDir const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << rhs.GetBase();
  lhs.imbue(old);
  return lhs;
}
}

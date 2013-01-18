// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/pelib/tls_dir.hpp"

#include <array>
#include <cstddef>
#include <ostream>
#include <utility>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/assert.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include <windows.h>
#include <winnt.h>

#include "hadesmem/read.hpp"
#include "hadesmem/error.hpp"
#include "hadesmem/write.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/pelib/pe_file.hpp"
#include "hadesmem/pelib/nt_headers.hpp"

namespace hadesmem
{

struct TlsDir::Impl
{
  explicit Impl(Process const& process, PeFile const& pe_file)
    : process_(&process), 
    pe_file_(&pe_file), 
    base_(nullptr)
  {
    NtHeaders const nt_headers(process, pe_file);

    // TODO: Some sort of API to handle this common case.
    DWORD const data_dir_size = nt_headers.GetDataDirectorySize(
      PeDataDir::TLS);
    DWORD const data_dir_va = nt_headers.GetDataDirectoryVirtualAddress(
      PeDataDir::TLS);
    if (!data_dir_size || !data_dir_va)
    {
      HADESMEM_THROW_EXCEPTION(Error() << 
        ErrorString("PE file has no TLS directory."));
    }

    base_ = static_cast<PBYTE>(RvaToVa(process, pe_file, data_dir_va));
  }

  Process const* process_;
  PeFile const* pe_file_;
  PBYTE base_;
};

TlsDir::TlsDir(Process const& process, PeFile const& pe_file)
  : impl_(new Impl(process, pe_file))
{ }

TlsDir::TlsDir(TlsDir const& other)
  : impl_(new Impl(*other.impl_))
{ }

TlsDir& TlsDir::operator=(TlsDir const& other)
{
  impl_ = std::unique_ptr<Impl>(new Impl(*other.impl_));

  return *this;
}

TlsDir::TlsDir(TlsDir&& other) HADESMEM_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

TlsDir& TlsDir::operator=(TlsDir&& other) HADESMEM_NOEXCEPT
{
  impl_ = std::move(other.impl_);
  
  return *this;
}

TlsDir::~TlsDir()
{ }

PVOID TlsDir::GetBase() const HADESMEM_NOEXCEPT
{
  return impl_->base_;
}

DWORD_PTR TlsDir::GetStartAddressOfRawData() const
{
  return Read<DWORD_PTR>(*impl_->process_, impl_->base_ + 
    offsetof(IMAGE_TLS_DIRECTORY, StartAddressOfRawData));
}

DWORD_PTR TlsDir::GetEndAddressOfRawData() const
{
  return Read<DWORD_PTR>(*impl_->process_, impl_->base_ + 
    offsetof(IMAGE_TLS_DIRECTORY, EndAddressOfRawData));
}

DWORD_PTR TlsDir::GetAddressOfIndex() const
{
  return Read<DWORD_PTR>(*impl_->process_, impl_->base_ + 
    offsetof(IMAGE_TLS_DIRECTORY, AddressOfIndex));
}

DWORD_PTR TlsDir::GetAddressOfCallBacks() const
{
  return Read<DWORD_PTR>(*impl_->process_, impl_->base_ + 
    offsetof(IMAGE_TLS_DIRECTORY, AddressOfCallBacks));
}

DWORD TlsDir::GetSizeOfZeroFill() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + 
    offsetof(IMAGE_TLS_DIRECTORY, SizeOfZeroFill));
}

DWORD TlsDir::GetCharacteristics() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + 
    offsetof(IMAGE_TLS_DIRECTORY, Characteristics));
}

std::vector<PIMAGE_TLS_CALLBACK> TlsDir::GetCallbacks() const
{
  std::vector<PIMAGE_TLS_CALLBACK> callbacks;

  NtHeaders nt_headers(*impl_->process_, *impl_->pe_file_);

  auto callbacks_raw = reinterpret_cast<PIMAGE_TLS_CALLBACK*>(RvaToVa(
    *impl_->process_, *impl_->pe_file_, static_cast<DWORD>(
    GetAddressOfCallBacks() - nt_headers.GetImageBase())));
  
  for (auto callback = Read<PIMAGE_TLS_CALLBACK>(*impl_->process_, 
    callbacks_raw); 
    callback; 
    callback = Read<PIMAGE_TLS_CALLBACK>(*impl_->process_, ++callbacks_raw))
  {
    DWORD_PTR const callback_offset = reinterpret_cast<DWORD_PTR>(callback) - 
      nt_headers.GetImageBase();
    callbacks.push_back(reinterpret_cast<PIMAGE_TLS_CALLBACK>(
      callback_offset));
  }
  
  return callbacks;
}

void TlsDir::SetStartAddressOfRawData(DWORD_PTR start_address_of_raw_data) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_TLS_DIRECTORY, 
    StartAddressOfRawData), start_address_of_raw_data);
}

void TlsDir::SetEndAddressOfRawData(DWORD_PTR end_address_of_raw_data) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_TLS_DIRECTORY, 
    EndAddressOfRawData), end_address_of_raw_data);
}

void TlsDir::SetAddressOfIndex(DWORD_PTR address_of_index) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_TLS_DIRECTORY, 
    AddressOfIndex), address_of_index);
}

void TlsDir::SetAddressOfCallBacks(DWORD_PTR address_of_callbacks) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_TLS_DIRECTORY, 
    AddressOfCallBacks), address_of_callbacks);
}

void TlsDir::SetSizeOfZeroFill(DWORD size_of_zero_fill) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_TLS_DIRECTORY, 
    SizeOfZeroFill), size_of_zero_fill);
}

void TlsDir::SetCharacteristics(DWORD characteristics) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_TLS_DIRECTORY, 
    Characteristics), characteristics);
}

bool operator==(TlsDir const& lhs, TlsDir const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

bool operator!=(TlsDir const& lhs, TlsDir const& rhs) HADESMEM_NOEXCEPT
{
  return !(lhs == rhs);
}

bool operator<(TlsDir const& lhs, TlsDir const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

bool operator<=(TlsDir const& lhs, TlsDir const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

bool operator>(TlsDir const& lhs, TlsDir const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

bool operator>=(TlsDir const& lhs, TlsDir const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

std::ostream& operator<<(std::ostream& lhs, TlsDir const& rhs)
{
  return (lhs << rhs.GetBase());
}

std::wostream& operator<<(std::wostream& lhs, TlsDir const& rhs)
{
  return (lhs << rhs.GetBase());
}

}

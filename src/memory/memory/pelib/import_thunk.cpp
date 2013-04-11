// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/pelib/import_thunk.hpp>

#include <cstddef>
#include <ostream>
#include <utility>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/assert.hpp>
#include <boost/lexical_cast.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <windows.h>
#include <winnt.h>

#include <hadesmem/read.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/write.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/pelib/import_dir.hpp>
#include <hadesmem/pelib/nt_headers.hpp>

// TODO: Fix the code so this hack can be removed.
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic ignored "-Wextended-offsetof"
#endif

namespace hadesmem
{

struct ImportThunk::Impl
{
  explicit Impl(Process const& process, PeFile const& pe_file, 
    PIMAGE_THUNK_DATA thunk)
    : process_(&process), 
    pe_file_(&pe_file), 
    base_(reinterpret_cast<PBYTE>(thunk))
  { }

  Process const* process_;
  PeFile const* pe_file_;
  PBYTE base_;
};

ImportThunk::ImportThunk(Process const& process, PeFile const& pe_file, 
  PIMAGE_THUNK_DATA thunk)
  : impl_(new Impl(process, pe_file, thunk))
{ }

ImportThunk::ImportThunk(ImportThunk const& other)
  : impl_(new Impl(*other.impl_))
{ }

ImportThunk& ImportThunk::operator=(ImportThunk const& other)
{
  impl_ = std::unique_ptr<Impl>(new Impl(*other.impl_));

  return *this;
}

ImportThunk::ImportThunk(ImportThunk&& other) HADESMEM_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

ImportThunk& ImportThunk::operator=(ImportThunk&& other) HADESMEM_NOEXCEPT
{
  impl_ = std::move(other.impl_);
  
  return *this;
}

ImportThunk::~ImportThunk()
{ }

PVOID ImportThunk::GetBase() const HADESMEM_NOEXCEPT
{
  return impl_->base_;
}

DWORD_PTR ImportThunk::GetAddressOfData() const
{
  return Read<DWORD_PTR>(*impl_->process_, impl_->base_ + FIELD_OFFSET(
    IMAGE_THUNK_DATA, u1.AddressOfData));
}

void ImportThunk::SetAddressOfData(DWORD_PTR address_of_data)
{
  return Write(*impl_->process_, impl_->base_ + FIELD_OFFSET(IMAGE_THUNK_DATA, 
    u1.AddressOfData), address_of_data);
}

DWORD_PTR ImportThunk::GetOrdinalRaw() const
{
  return Read<DWORD_PTR>(*impl_->process_, impl_->base_ + FIELD_OFFSET(
    IMAGE_THUNK_DATA, u1.Ordinal));
}

void ImportThunk::SetOrdinalRaw(DWORD_PTR ordinal_raw)
{
  return Write(*impl_->process_, impl_->base_ + FIELD_OFFSET(IMAGE_THUNK_DATA, 
    u1.Ordinal), ordinal_raw);
}

bool ImportThunk::ByOrdinal() const
{
  return IMAGE_SNAP_BY_ORDINAL(GetOrdinalRaw());
}

WORD ImportThunk::GetOrdinal() const
{
  return IMAGE_ORDINAL(GetOrdinalRaw());
}

DWORD_PTR ImportThunk::GetFunction() const
{
  return Read<DWORD_PTR>(*impl_->process_, impl_->base_ + FIELD_OFFSET(
    IMAGE_THUNK_DATA, u1.Function));
}

void ImportThunk::SetFunction(DWORD_PTR function)
{
  return Write(*impl_->process_, impl_->base_ + FIELD_OFFSET(IMAGE_THUNK_DATA, 
    u1.Function), function);
}

WORD ImportThunk::GetHint() const
{
  PBYTE const name_import = static_cast<PBYTE>(RvaToVa(*impl_->process_, 
    *impl_->pe_file_, static_cast<DWORD>(GetAddressOfData())));
  return Read<WORD>(*impl_->process_, name_import + FIELD_OFFSET(
    IMAGE_IMPORT_BY_NAME, Hint));
}

void ImportThunk::SetHint(WORD hint)
{
  PBYTE const name_import = static_cast<PBYTE>(RvaToVa(*impl_->process_, 
    *impl_->pe_file_, static_cast<DWORD>(GetAddressOfData())));
  return Write(*impl_->process_, name_import + FIELD_OFFSET(
    IMAGE_IMPORT_BY_NAME, Hint), hint);
}

std::string ImportThunk::GetName() const
{
  PBYTE const name_import = static_cast<PBYTE>(RvaToVa(*impl_->process_, 
    *impl_->pe_file_, static_cast<DWORD>(GetAddressOfData())));
  return ReadString<char>(*impl_->process_, name_import + FIELD_OFFSET(
    IMAGE_IMPORT_BY_NAME, Name));
}

bool operator==(ImportThunk const& lhs, ImportThunk const& rhs) 
  HADESMEM_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

bool operator!=(ImportThunk const& lhs, ImportThunk const& rhs) 
  HADESMEM_NOEXCEPT
{
  return !(lhs == rhs);
}

bool operator<(ImportThunk const& lhs, ImportThunk const& rhs) 
  HADESMEM_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

bool operator<=(ImportThunk const& lhs, ImportThunk const& rhs) 
  HADESMEM_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

bool operator>(ImportThunk const& lhs, ImportThunk const& rhs) 
  HADESMEM_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

bool operator>=(ImportThunk const& lhs, ImportThunk const& rhs) 
  HADESMEM_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

std::ostream& operator<<(std::ostream& lhs, ImportThunk const& rhs)
{
  return (lhs << rhs.GetBase());
}

std::wostream& operator<<(std::wostream& lhs, ImportThunk const& rhs)
{
  return (lhs << rhs.GetBase());
}

}

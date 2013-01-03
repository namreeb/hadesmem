// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/pelib/nt_headers.hpp"

#include <cstddef>
#include <utility>
#include <iostream>
#include <type_traits>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/assert.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include <windows.h>
#include <winnt.h>

#include "hadesmem/read.hpp"
#include "hadesmem/error.hpp"
#include "hadesmem/write.hpp"
#include "hadesmem/config.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/pelib/pe_file.hpp"
#include "hadesmem/pelib/dos_header.hpp"

// TODO: Fix the code so this hack can be removed.
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic ignored "-Wextended-offsetof"
#endif

namespace hadesmem
{

struct NtHeaders::Impl
{
  explicit Impl(Process const& process, PeFile const& pe_file)
    : process_(&process), 
    pe_file_(&pe_file), 
    base_(nullptr)
  {
    DosHeader dos_header(process, pe_file);
    dos_header.EnsureValid();
    base_ = static_cast<PBYTE>(dos_header.GetBase()) + 
      dos_header.GetNewHeaderOffset();
  }

  Process const* process_;
  PeFile const* pe_file_;
  PBYTE base_;
};

NtHeaders::NtHeaders(Process const& process, PeFile const& pefile)
  : impl_(new Impl(process, pefile))
{ }

NtHeaders::NtHeaders(NtHeaders const& other)
  : impl_(new Impl(*other.impl_))
{ }

NtHeaders& NtHeaders::operator=(NtHeaders const& other)
{
  impl_ = std::unique_ptr<Impl>(new Impl(*other.impl_));

  return *this;
}

NtHeaders::NtHeaders(NtHeaders&& other) HADESMEM_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

NtHeaders& NtHeaders::operator=(NtHeaders&& other) HADESMEM_NOEXCEPT
{
  impl_ = std::move(other.impl_);
  
  return *this;
}

NtHeaders::~NtHeaders()
{ }

PVOID NtHeaders::GetBase() const HADESMEM_NOEXCEPT
{
  return impl_->base_;
}

bool NtHeaders::IsValid() const
{
  return IMAGE_NT_SIGNATURE == GetSignature();
}

void NtHeaders::EnsureValid() const
{
  if (!IsValid())
  {
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("NT headers signature invalid."));
  }
}

DWORD NtHeaders::GetSignature() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, Signature));
}

WORD NtHeaders::GetMachine() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, FileHeader.Machine));
}

WORD NtHeaders::GetNumberOfSections() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, FileHeader.NumberOfSections));
}

DWORD NtHeaders::GetTimeDateStamp() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, FileHeader.TimeDateStamp));
}

DWORD NtHeaders::GetPointerToSymbolTable() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, FileHeader.PointerToSymbolTable));
}

DWORD NtHeaders::GetNumberOfSymbols() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, FileHeader.NumberOfSymbols));
}

WORD NtHeaders::GetSizeOfOptionalHeader() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, FileHeader.SizeOfOptionalHeader));
}

WORD NtHeaders::GetCharacteristics() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, FileHeader.Characteristics));
}

WORD NtHeaders::GetMagic() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.Magic));
}

BYTE NtHeaders::GetMajorLinkerVersion() const
{
  return Read<BYTE>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.MajorLinkerVersion));
}

BYTE NtHeaders::GetMinorLinkerVersion() const
{
  return Read<BYTE>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.MinorLinkerVersion));
}

DWORD NtHeaders::GetSizeOfCode() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.SizeOfCode));
}

DWORD NtHeaders::GetSizeOfInitializedData() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.SizeOfInitializedData));
}

DWORD NtHeaders::GetSizeOfUninitializedData() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.SizeOfUninitializedData));
}

DWORD NtHeaders::GetAddressOfEntryPoint() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.AddressOfEntryPoint));
}

DWORD NtHeaders::GetBaseOfCode() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.BaseOfCode));
}

#if defined(_M_IX86) 
DWORD NtHeaders::GetBaseOfData() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.BaseOfData));
}
#endif

ULONG_PTR NtHeaders::GetImageBase() const
{
  return Read<ULONG_PTR>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.ImageBase));
}

DWORD NtHeaders::GetSectionAlignment() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.SectionAlignment));
}

DWORD NtHeaders::GetFileAlignment() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.FileAlignment));
}

WORD NtHeaders::GetMajorOperatingSystemVersion() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.MajorOperatingSystemVersion));
}

WORD NtHeaders::GetMinorOperatingSystemVersion() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.MinorOperatingSystemVersion));
}

WORD NtHeaders::GetMajorImageVersion() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.MajorImageVersion));
}

WORD NtHeaders::GetMinorImageVersion() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.MinorImageVersion));
}

WORD NtHeaders::GetMajorSubsystemVersion() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.MajorSubsystemVersion));
}

WORD NtHeaders::GetMinorSubsystemVersion() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.MinorSubsystemVersion));
}

DWORD NtHeaders::GetWin32VersionValue() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.Win32VersionValue));
}

DWORD NtHeaders::GetSizeOfImage() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.SizeOfImage));
}

DWORD NtHeaders::GetSizeOfHeaders() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.SizeOfHeaders));
}

DWORD NtHeaders::GetCheckSum() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.CheckSum));
}

WORD NtHeaders::GetSubsystem() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.Subsystem));
}

WORD NtHeaders::GetDllCharacteristics() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.DllCharacteristics));
}

ULONG_PTR NtHeaders::GetSizeOfStackReserve() const
{
  return Read<ULONG_PTR>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.SizeOfStackReserve));
}

ULONG_PTR NtHeaders::GetSizeOfStackCommit() const
{
  return Read<ULONG_PTR>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.SizeOfStackCommit));
}

ULONG_PTR NtHeaders::GetSizeOfHeapReserve() const
{
  return Read<ULONG_PTR>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.SizeOfHeapReserve));
}

ULONG_PTR NtHeaders::GetSizeOfHeapCommit() const
{
  return Read<ULONG_PTR>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.SizeOfHeapCommit));
}

DWORD NtHeaders::GetLoaderFlags() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.LoaderFlags));
}

DWORD NtHeaders::GetNumberOfRvaAndSizes() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.NumberOfRvaAndSizes));
}

DWORD NtHeaders::GetDataDirectoryVirtualAddress(PeDataDir data_dir) const
{
  if (static_cast<DWORD>(data_dir) >= GetNumberOfRvaAndSizes())
  {
    return 0;
  }

  return Read<DWORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.DataDirectory[0]) + 
    static_cast<detail::UnderlyingType<PeDataDir>::type>(data_dir) * 
    sizeof(IMAGE_DATA_DIRECTORY) + offsetof(IMAGE_DATA_DIRECTORY, 
    VirtualAddress));
}

DWORD NtHeaders::GetDataDirectorySize(PeDataDir data_dir) const
{
  if (static_cast<DWORD>(data_dir) >= GetNumberOfRvaAndSizes())
  {
    return 0;
  }

  return Read<DWORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_NT_HEADERS, OptionalHeader.DataDirectory[0]) + 
    static_cast<detail::UnderlyingType<PeDataDir>::type>(data_dir) * 
    sizeof(IMAGE_DATA_DIRECTORY) + offsetof(IMAGE_DATA_DIRECTORY, Size));
}

void NtHeaders::SetSignature(DWORD signature) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, Signature), 
    signature);
}

void NtHeaders::SetMachine(WORD machine) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, FileHeader.
    Machine), machine);
}

void NtHeaders::SetNumberOfSections(WORD number_of_sections) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, FileHeader.
    NumberOfSections), number_of_sections);
}

void NtHeaders::SetTimeDateStamp(DWORD time_date_stamp) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, FileHeader.
    TimeDateStamp), time_date_stamp);
}

void NtHeaders::SetPointerToSymbolTable(DWORD pointer_to_symbol_table) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, FileHeader.
    PointerToSymbolTable), pointer_to_symbol_table);
}

void NtHeaders::SetNumberOfSymbols(DWORD number_of_symbols) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, FileHeader.
    NumberOfSymbols), number_of_symbols);
}

void NtHeaders::SetSizeOfOptionalHeader(WORD size_of_optional_header) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, FileHeader.
    SizeOfOptionalHeader), size_of_optional_header);

}

void NtHeaders::SetCharacteristics(WORD characteristics) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, FileHeader.
    Characteristics), characteristics);
}

void NtHeaders::SetMagic(WORD magic) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.Magic), magic);
}

void NtHeaders::SetMajorLinkerVersion(BYTE major_linker_version) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.MajorLinkerVersion), major_linker_version);
}

void NtHeaders::SetMinorLinkerVersion(BYTE minor_linker_version) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.MinorLinkerVersion), minor_linker_version);
}

void NtHeaders::SetSizeOfCode(DWORD size_of_code) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.SizeOfCode), size_of_code);
}

void NtHeaders::SetSizeOfInitializedData(DWORD size_of_initialized_data) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.SizeOfInitializedData), size_of_initialized_data);
}

void NtHeaders::SetSizeOfUninitializedData(DWORD size_of_uninitialized_data) 
  const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.SizeOfUninitializedData), size_of_uninitialized_data);
}

void NtHeaders::SetAddressOfEntryPoint(DWORD address_of_entry_point) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.AddressOfEntryPoint), address_of_entry_point);
}

void NtHeaders::SetBaseOfCode(DWORD base_of_code) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.BaseOfCode), base_of_code);
}

#if defined(_M_IX86) 
void NtHeaders::SetBaseOfData(DWORD base_of_data) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.BaseOfData), base_of_data);
}
#endif

void NtHeaders::SetImageBase(ULONG_PTR image_base) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.ImageBase), image_base);
}

void NtHeaders::SetSectionAlignment(DWORD section_alignment) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.SectionAlignment), section_alignment);
}

void NtHeaders::SetFileAlignment(DWORD file_alignment) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.FileAlignment), file_alignment);
}

void NtHeaders::SetMajorOperatingSystemVersion(
  WORD major_operating_system_version) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.MajorOperatingSystemVersion), 
    major_operating_system_version);
}

void NtHeaders::SetMinorOperatingSystemVersion(
  WORD minor_operating_system_version) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.MinorOperatingSystemVersion), 
    minor_operating_system_version);
}

void NtHeaders::SetMajorImageVersion(WORD major_image_version) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.MajorImageVersion), major_image_version);
}

void NtHeaders::SetMinorImageVersion(WORD minor_image_version) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.MinorImageVersion), minor_image_version);
}

void NtHeaders::SetMajorSubsystemVersion(WORD major_subsystem_version) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.MajorSubsystemVersion), major_subsystem_version);
}

void NtHeaders::SetMinorSubsystemVersion(WORD minor_subsystem_version) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.MinorSubsystemVersion), minor_subsystem_version);
}

void NtHeaders::SetWin32VersionValue(DWORD win32_version_value) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.Win32VersionValue), win32_version_value);
}

void NtHeaders::SetSizeOfImage(DWORD size_of_image) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.SizeOfImage), size_of_image);
}

void NtHeaders::SetSizeOfHeaders(DWORD size_of_headers) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.SizeOfHeaders), size_of_headers);
}

void NtHeaders::SetCheckSum(DWORD checksum) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.CheckSum), checksum);
}

void NtHeaders::SetSubsystem(WORD subsystem) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.Subsystem), subsystem);
}

void NtHeaders::SetDllCharacteristics(WORD dll_characteristics) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.DllCharacteristics), dll_characteristics);
}

void NtHeaders::SetSizeOfStackReserve(ULONG_PTR size_of_stack_reserve) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.SizeOfStackReserve), size_of_stack_reserve);
}

void NtHeaders::SetSizeOfStackCommit(ULONG_PTR size_of_stack_commit) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.SizeOfStackCommit), size_of_stack_commit);
}

void NtHeaders::SetSizeOfHeapReserve(ULONG_PTR size_of_heap_reserve) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.SizeOfHeapReserve), size_of_heap_reserve);
}

void NtHeaders::SetSizeOfHeapCommit(ULONG_PTR size_of_heap_commit) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.SizeOfHeapCommit), size_of_heap_commit);
}

void NtHeaders::SetLoaderFlags(DWORD loader_flags) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.LoaderFlags), loader_flags);
}

void NtHeaders::SetNumberOfRvaAndSizes(DWORD number_of_rva_and_sizes) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.NumberOfRvaAndSizes), number_of_rva_and_sizes);
}

void NtHeaders::SetDataDirectoryVirtualAddress(PeDataDir data_dir, 
  DWORD data_directory_virtual_address) const
{
  if (static_cast<DWORD>(data_dir) >= GetNumberOfRvaAndSizes())
  {
    return;
  }

  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.DataDirectory[0]) + 
    static_cast<detail::UnderlyingType<PeDataDir>::type>(data_dir) * 
    sizeof(IMAGE_DATA_DIRECTORY) + offsetof(IMAGE_DATA_DIRECTORY, 
    VirtualAddress), data_directory_virtual_address);
}

void NtHeaders::SetDataDirectorySize(PeDataDir data_dir, 
  DWORD data_directory_size) const
{
  if (static_cast<DWORD>(data_dir) >= GetNumberOfRvaAndSizes())
  {
    return;
  }

  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_NT_HEADERS, 
    OptionalHeader.DataDirectory[0]) + 
    static_cast<detail::UnderlyingType<PeDataDir>::type>(data_dir) * 
    sizeof(IMAGE_DATA_DIRECTORY) + offsetof(IMAGE_DATA_DIRECTORY, Size), 
    data_directory_size);
}

bool operator==(NtHeaders const& lhs, NtHeaders const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

bool operator!=(NtHeaders const& lhs, NtHeaders const& rhs) HADESMEM_NOEXCEPT
{
  return !(lhs == rhs);
}

bool operator<(NtHeaders const& lhs, NtHeaders const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

bool operator<=(NtHeaders const& lhs, NtHeaders const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

bool operator>(NtHeaders const& lhs, NtHeaders const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

bool operator>=(NtHeaders const& lhs, NtHeaders const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

std::ostream& operator<<(std::ostream& lhs, NtHeaders const& rhs)
{
  return (lhs << rhs.GetBase());
}

std::wostream& operator<<(std::wostream& lhs, NtHeaders const& rhs)
{
  return (lhs << rhs.GetBase());
}

}

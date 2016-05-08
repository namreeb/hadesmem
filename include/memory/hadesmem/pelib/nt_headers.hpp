// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <ostream>
#include <utility>

#include <windows.h>
#include <winnt.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/pelib/dos_header.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

namespace hadesmem
{
enum class PeDataDir : std::uint32_t
{
  Export,
  Import,
  Resource,
  Exception,
  Security,
  BaseReloc,
  Debug,
  Architecture,
  GlobalPTR,
  TLS,
  LoadConfig,
  BoundImport,
  IAT,
  DelayImport,
  COMDescriptor,
  Reserved
};

class NtHeaders
{
public:
  explicit NtHeaders(Process const& process, PeFile const& pe_file)
    : process_{&process},
      pe_file_{&pe_file},
      base_{CalculateBase(*process_, *pe_file_)}
  {
    UpdateRead();

    EnsureValid();
  }

  explicit NtHeaders(Process const&& process, PeFile const& pe_file) = delete;

  explicit NtHeaders(Process const& process, PeFile&& pe_file) = delete;

  explicit NtHeaders(Process const&& process, PeFile&& pe_file) = delete;

  PVOID GetBase() const noexcept
  {
    return base_;
  }

  void UpdateRead()
  {
    if (pe_file_->Is64())
    {
      data_64_ = Read<IMAGE_NT_HEADERS64>(*process_, base_);
    }
    else
    {
      data_32_ = Read<IMAGE_NT_HEADERS32>(*process_, base_);
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

  bool IsValid() const
  {
    if (pe_file_->Is64())
    {
      return IMAGE_NT_SIGNATURE == GetSignature() &&
             IMAGE_NT_OPTIONAL_HDR64_MAGIC == GetMagic() &&
             IMAGE_FILE_MACHINE_AMD64 == GetMachine();
    }
    else
    {
      return IMAGE_NT_SIGNATURE == GetSignature() &&
             IMAGE_NT_OPTIONAL_HDR32_MAGIC == GetMagic() &&
             IMAGE_FILE_MACHINE_I386 == GetMachine();
    }
  }

  void EnsureValid() const
  {
    if (!IsValid())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error{} << ErrorString{"NT headers signature invalid."});
    }
  }

  DWORD GetSignature() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.Signature;
    }
    else
    {
      return data_32_.Signature;
    }
  }

  WORD GetMachine() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.FileHeader.Machine;
    }
    else
    {
      return data_32_.FileHeader.Machine;
    }
  }

  WORD GetNumberOfSections() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.FileHeader.NumberOfSections;
    }
    else
    {
      return data_32_.FileHeader.NumberOfSections;
    }
  }

  DWORD GetTimeDateStamp() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.FileHeader.TimeDateStamp;
    }
    else
    {
      return data_32_.FileHeader.TimeDateStamp;
    }
  }

  DWORD GetPointerToSymbolTable() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.FileHeader.PointerToSymbolTable;
    }
    else
    {
      return data_32_.FileHeader.PointerToSymbolTable;
    }
  }

  DWORD GetNumberOfSymbols() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.FileHeader.NumberOfSymbols;
    }
    else
    {
      return data_32_.FileHeader.NumberOfSymbols;
    }
  }

  WORD GetSizeOfOptionalHeader() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.FileHeader.SizeOfOptionalHeader;
    }
    else
    {
      return data_32_.FileHeader.SizeOfOptionalHeader;
    }
  }

  WORD GetCharacteristics() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.FileHeader.Characteristics;
    }
    else
    {
      return data_32_.FileHeader.Characteristics;
    }
  }

  WORD GetMagic() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader.Magic;
    }
    else
    {
      return data_32_.OptionalHeader.Magic;
    }
  }

  BYTE GetMajorLinkerVersion() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader.MajorLinkerVersion;
    }
    else
    {
      return data_32_.OptionalHeader.MajorLinkerVersion;
    }
  }

  BYTE GetMinorLinkerVersion() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader.MinorLinkerVersion;
    }
    else
    {
      return data_32_.OptionalHeader.MinorLinkerVersion;
    }
  }

  DWORD GetSizeOfCode() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader.SizeOfCode;
    }
    else
    {
      return data_32_.OptionalHeader.SizeOfCode;
    }
  }

  DWORD GetSizeOfInitializedData() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader.SizeOfInitializedData;
    }
    else
    {
      return data_32_.OptionalHeader.SizeOfInitializedData;
    }
  }

  DWORD GetSizeOfUninitializedData() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader.SizeOfUninitializedData;
    }
    else
    {
      return data_32_.OptionalHeader.SizeOfUninitializedData;
    }
  }

  DWORD GetAddressOfEntryPoint() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader.AddressOfEntryPoint;
    }
    else
    {
      return data_32_.OptionalHeader.AddressOfEntryPoint;
    }
  }

  DWORD GetBaseOfCode() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader.BaseOfCode;
    }
    else
    {
      return data_32_.OptionalHeader.BaseOfCode;
    }
  }

  DWORD GetBaseOfData() const
  {
    if (pe_file_->Is64())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error{} << ErrorString{"Invalid field for architecture."});
    }
    else
    {
      return data_32_.OptionalHeader.BaseOfData;
    }
  }

  ULONGLONG GetImageBase() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader.ImageBase;
    }
    else
    {
      return data_32_.OptionalHeader.ImageBase;
    }
  }

  DWORD GetSectionAlignment() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader.SectionAlignment;
    }
    else
    {
      return data_32_.OptionalHeader.SectionAlignment;
    }
  }

  DWORD GetFileAlignment() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader.FileAlignment;
    }
    else
    {
      return data_32_.OptionalHeader.FileAlignment;
    }
  }

  WORD GetMajorOperatingSystemVersion() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader.MajorOperatingSystemVersion;
    }
    else
    {
      return data_32_.OptionalHeader.MajorOperatingSystemVersion;
    }
  }

  WORD GetMinorOperatingSystemVersion() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader.MinorOperatingSystemVersion;
    }
    else
    {
      return data_32_.OptionalHeader.MinorOperatingSystemVersion;
    }
  }

  WORD GetMajorImageVersion() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader.MajorImageVersion;
    }
    else
    {
      return data_32_.OptionalHeader.MajorImageVersion;
    }
  }

  WORD GetMinorImageVersion() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader.MinorImageVersion;
    }
    else
    {
      return data_32_.OptionalHeader.MinorImageVersion;
    }
  }

  WORD GetMajorSubsystemVersion() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader.MajorSubsystemVersion;
    }
    else
    {
      return data_32_.OptionalHeader.MajorSubsystemVersion;
    }
  }

  WORD GetMinorSubsystemVersion() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader.MinorSubsystemVersion;
    }
    else
    {
      return data_32_.OptionalHeader.MinorSubsystemVersion;
    }
  }

  DWORD GetWin32VersionValue() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader.Win32VersionValue;
    }
    else
    {
      return data_32_.OptionalHeader.Win32VersionValue;
    }
  }

  DWORD GetSizeOfImage() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader.SizeOfImage;
    }
    else
    {
      return data_32_.OptionalHeader.SizeOfImage;
    }
  }

  DWORD GetSizeOfHeaders() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader.SizeOfHeaders;
    }
    else
    {
      return data_32_.OptionalHeader.SizeOfHeaders;
    }
  }

  DWORD GetCheckSum() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader.CheckSum;
    }
    else
    {
      return data_32_.OptionalHeader.CheckSum;
    }
  }

  WORD GetSubsystem() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader.Subsystem;
    }
    else
    {
      return data_32_.OptionalHeader.Subsystem;
    }
  }

  WORD GetDllCharacteristics() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader.DllCharacteristics;
    }
    else
    {
      return data_32_.OptionalHeader.DllCharacteristics;
    }
  }

  ULONGLONG GetSizeOfStackReserve() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader.SizeOfStackReserve;
    }
    else
    {
      return data_32_.OptionalHeader.SizeOfStackReserve;
    }
  }

  ULONGLONG GetSizeOfStackCommit() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader.SizeOfStackCommit;
    }
    else
    {
      return data_32_.OptionalHeader.SizeOfStackCommit;
    }
  }

  ULONGLONG GetSizeOfHeapReserve() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader.SizeOfHeapReserve;
    }
    else
    {
      return data_32_.OptionalHeader.SizeOfHeapReserve;
    }
  }

  ULONGLONG GetSizeOfHeapCommit() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader.SizeOfHeapCommit;
    }
    else
    {
      return data_32_.OptionalHeader.SizeOfHeapCommit;
    }
  }

  DWORD GetLoaderFlags() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader.LoaderFlags;
    }
    else
    {
      return data_32_.OptionalHeader.LoaderFlags;
    }
  }

  DWORD GetNumberOfRvaAndSizes() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader.NumberOfRvaAndSizes;
    }
    else
    {
      return data_32_.OptionalHeader.NumberOfRvaAndSizes;
    }
  }

  DWORD GetNumberOfRvaAndSizesClamped() const
  {
    DWORD const num_rvas_and_sizes = GetNumberOfRvaAndSizes();
    return (std::min)(num_rvas_and_sizes, 0x10UL);
  }

  DWORD GetDataDirectoryVirtualAddress(PeDataDir data_dir) const
  {
    if (static_cast<DWORD>(data_dir) >= GetNumberOfRvaAndSizesClamped())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                      << ErrorString{"Invalid data dir."});
    }

    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader
        .DataDirectory[static_cast<std::uint32_t>(data_dir)]
        .VirtualAddress;
    }
    else
    {
      return data_32_.OptionalHeader
        .DataDirectory[static_cast<std::uint32_t>(data_dir)]
        .VirtualAddress;
    }
  }

  DWORD GetDataDirectorySize(PeDataDir data_dir) const
  {
    if (static_cast<DWORD>(data_dir) >= GetNumberOfRvaAndSizesClamped())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                      << ErrorString{"Invalid data dir."});
    }

    if (pe_file_->Is64())
    {
      return data_64_.OptionalHeader
        .DataDirectory[static_cast<std::uint32_t>(data_dir)]
        .Size;
    }
    else
    {
      return data_32_.OptionalHeader
        .DataDirectory[static_cast<std::uint32_t>(data_dir)]
        .Size;
    }
  }

  void SetSignature(DWORD signature)
  {
    if (pe_file_->Is64())
    {
      data_64_.Signature = signature;
    }
    else
    {
      data_32_.Signature = signature;
    }
  }

  void SetMachine(WORD machine)
  {
    if (pe_file_->Is64())
    {
      data_64_.FileHeader.Machine = machine;
    }
    else
    {
      data_32_.FileHeader.Machine = machine;
    }
  }

  void SetNumberOfSections(WORD number_of_sections)
  {
    if (pe_file_->Is64())
    {
      data_64_.FileHeader.NumberOfSections = number_of_sections;
    }
    else
    {
      data_32_.FileHeader.NumberOfSections = number_of_sections;
    }
  }

  void SetTimeDateStamp(DWORD time_date_stamp)
  {
    if (pe_file_->Is64())
    {
      data_64_.FileHeader.TimeDateStamp = time_date_stamp;
    }
    else
    {
      data_32_.FileHeader.TimeDateStamp = time_date_stamp;
    }
  }

  void SetPointerToSymbolTable(DWORD pointer_to_symbol_table)
  {
    if (pe_file_->Is64())
    {
      data_64_.FileHeader.PointerToSymbolTable = pointer_to_symbol_table;
    }
    else
    {
      data_32_.FileHeader.PointerToSymbolTable = pointer_to_symbol_table;
    }
  }

  void SetNumberOfSymbols(DWORD number_of_symbols)
  {
    if (pe_file_->Is64())
    {
      data_64_.FileHeader.NumberOfSymbols = number_of_symbols;
    }
    else
    {
      data_32_.FileHeader.NumberOfSymbols = number_of_symbols;
    }
  }

  void SetSizeOfOptionalHeader(WORD size_of_optional_header)
  {
    if (pe_file_->Is64())
    {
      data_64_.FileHeader.SizeOfOptionalHeader = size_of_optional_header;
    }
    else
    {
      data_32_.FileHeader.SizeOfOptionalHeader = size_of_optional_header;
    }
  }

  void SetCharacteristics(WORD characteristics)
  {
    if (pe_file_->Is64())
    {
      data_64_.FileHeader.Characteristics = characteristics;
    }
    else
    {
      data_32_.FileHeader.Characteristics = characteristics;
    }
  }

  void SetMagic(WORD magic)
  {
    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader.Magic = magic;
    }
    else
    {
      data_32_.OptionalHeader.Magic = magic;
    }
  }

  void SetMajorLinkerVersion(BYTE major_linker_version)
  {
    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader.MajorLinkerVersion = major_linker_version;
    }
    else
    {
      data_32_.OptionalHeader.MajorLinkerVersion = major_linker_version;
    }
  }

  void SetMinorLinkerVersion(BYTE minor_linker_version)
  {
    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader.MinorLinkerVersion = minor_linker_version;
    }
    else
    {
      data_32_.OptionalHeader.MinorLinkerVersion = minor_linker_version;
    }
  }

  void SetSizeOfCode(DWORD size_of_code)
  {
    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader.SizeOfCode = size_of_code;
    }
    else
    {
      data_32_.OptionalHeader.SizeOfCode = size_of_code;
    }
  }

  void SetSizeOfInitializedData(DWORD size_of_initialized_data)
  {
    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader.SizeOfInitializedData = size_of_initialized_data;
    }
    else
    {
      data_32_.OptionalHeader.SizeOfInitializedData = size_of_initialized_data;
    }
  }

  void SetSizeOfUninitializedData(DWORD size_of_uninitialized_data)
  {
    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader.SizeOfUninitializedData =
        size_of_uninitialized_data;
    }
    else
    {
      data_32_.OptionalHeader.SizeOfUninitializedData =
        size_of_uninitialized_data;
    }
  }

  void SetAddressOfEntryPoint(DWORD address_of_entry_point)
  {
    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader.AddressOfEntryPoint = address_of_entry_point;
    }
    else
    {
      data_32_.OptionalHeader.AddressOfEntryPoint = address_of_entry_point;
    }
  }

  void SetBaseOfCode(DWORD base_of_code)
  {
    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader.BaseOfCode = base_of_code;
    }
    else
    {
      data_32_.OptionalHeader.BaseOfCode = base_of_code;
    }
  }

  void SetBaseOfData(DWORD base_of_data)
  {
    if (pe_file_->Is64())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error{} << ErrorString{"Invalid field for architecture."});
    }
    else
    {
      data_32_.OptionalHeader.BaseOfData = base_of_data;
    }
  }

  void SetImageBase(ULONGLONG image_base)
  {
    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader.ImageBase = image_base;
    }
    else
    {
      data_32_.OptionalHeader.ImageBase = static_cast<DWORD>(image_base);
    }
  }

  void SetSectionAlignment(DWORD section_alignment)
  {
    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader.SectionAlignment = section_alignment;
    }
    else
    {
      data_32_.OptionalHeader.SectionAlignment = section_alignment;
    }
  }

  void SetFileAlignment(DWORD file_alignment)
  {
    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader.FileAlignment = file_alignment;
    }
    else
    {
      data_32_.OptionalHeader.FileAlignment = file_alignment;
    }
  }

  void SetMajorOperatingSystemVersion(WORD major_operating_system_version)
  {
    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader.MajorOperatingSystemVersion =
        major_operating_system_version;
    }
    else
    {
      data_32_.OptionalHeader.MajorOperatingSystemVersion =
        major_operating_system_version;
    }
  }

  void SetMinorOperatingSystemVersion(WORD minor_operating_system_version)
  {
    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader.MinorOperatingSystemVersion =
        minor_operating_system_version;
    }
    else
    {
      data_32_.OptionalHeader.MinorOperatingSystemVersion =
        minor_operating_system_version;
    }
  }

  void SetMajorImageVersion(WORD major_image_version)
  {
    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader.MajorImageVersion = major_image_version;
    }
    else
    {
      data_32_.OptionalHeader.MajorImageVersion = major_image_version;
    }
  }

  void SetMinorImageVersion(WORD minor_image_version)
  {
    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader.MinorImageVersion = minor_image_version;
    }
    else
    {
      data_32_.OptionalHeader.MinorImageVersion = minor_image_version;
    }
  }

  void SetMajorSubsystemVersion(WORD major_subsystem_version)
  {
    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader.MajorSubsystemVersion = major_subsystem_version;
    }
    else
    {
      data_32_.OptionalHeader.MajorSubsystemVersion = major_subsystem_version;
    }
  }

  void SetMinorSubsystemVersion(WORD minor_subsystem_version)
  {
    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader.MinorSubsystemVersion = minor_subsystem_version;
    }
    else
    {
      data_32_.OptionalHeader.MinorSubsystemVersion = minor_subsystem_version;
    }
  }

  void SetWin32VersionValue(DWORD win32_version_value)
  {
    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader.Win32VersionValue = win32_version_value;
    }
    else
    {
      data_32_.OptionalHeader.Win32VersionValue = win32_version_value;
    }
  }

  void SetSizeOfImage(DWORD size_of_image)
  {
    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader.SizeOfImage = size_of_image;
    }
    else
    {
      data_32_.OptionalHeader.SizeOfImage = size_of_image;
    }
  }

  void SetSizeOfHeaders(DWORD size_of_headers)
  {
    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader.SizeOfHeaders = size_of_headers;
    }
    else
    {
      data_32_.OptionalHeader.SizeOfHeaders = size_of_headers;
    }
  }

  void SetCheckSum(DWORD checksum)
  {
    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader.CheckSum = checksum;
    }
    else
    {
      data_32_.OptionalHeader.CheckSum = checksum;
    }
  }

  void SetSubsystem(WORD subsystem)
  {
    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader.Subsystem = subsystem;
    }
    else
    {
      data_32_.OptionalHeader.Subsystem = subsystem;
    }
  }

  void SetDllCharacteristics(WORD dll_characteristics)
  {
    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader.DllCharacteristics = dll_characteristics;
    }
    else
    {
      data_32_.OptionalHeader.DllCharacteristics = dll_characteristics;
    }
  }

  void SetSizeOfStackReserve(ULONGLONG size_of_stack_reserve)
  {
    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader.SizeOfStackReserve = size_of_stack_reserve;
    }
    else
    {
      data_32_.OptionalHeader.SizeOfStackReserve =
        static_cast<ULONG>(size_of_stack_reserve);
    }
  }

  void SetSizeOfStackCommit(ULONGLONG size_of_stack_commit)
  {
    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader.SizeOfStackCommit = size_of_stack_commit;
    }
    else
    {
      data_32_.OptionalHeader.SizeOfStackCommit =
        static_cast<ULONG>(size_of_stack_commit);
    }
  }

  void SetSizeOfHeapReserve(ULONGLONG size_of_heap_reserve)
  {
    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader.SizeOfHeapReserve = size_of_heap_reserve;
    }
    else
    {
      data_32_.OptionalHeader.SizeOfHeapReserve =
        static_cast<ULONG>(size_of_heap_reserve);
    }
  }

  void SetSizeOfHeapCommit(ULONGLONG size_of_heap_commit)
  {
    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader.SizeOfHeapCommit = size_of_heap_commit;
    }
    else
    {
      data_32_.OptionalHeader.SizeOfHeapCommit =
        static_cast<ULONG>(size_of_heap_commit);
    }
  }

  void SetLoaderFlags(DWORD loader_flags)
  {
    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader.LoaderFlags = loader_flags;
    }
    else
    {
      data_32_.OptionalHeader.LoaderFlags = loader_flags;
    }
  }

  void SetNumberOfRvaAndSizes(DWORD number_of_rva_and_sizes)
  {
    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader.NumberOfRvaAndSizes = number_of_rva_and_sizes;
    }
    else
    {
      data_32_.OptionalHeader.NumberOfRvaAndSizes = number_of_rva_and_sizes;
    }
  }

  void SetDataDirectoryVirtualAddress(PeDataDir data_dir,
                                      DWORD data_directory_virtual_address)
  {
    if (static_cast<DWORD>(data_dir) >= GetNumberOfRvaAndSizes())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                      << ErrorString{"Invalid data dir."});
    }

    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader
        .DataDirectory[static_cast<std::uint32_t>(data_dir)]
        .VirtualAddress = data_directory_virtual_address;
    }
    else
    {
      data_32_.OptionalHeader
        .DataDirectory[static_cast<std::uint32_t>(data_dir)]
        .VirtualAddress = data_directory_virtual_address;
    }
  }

  void SetDataDirectorySize(PeDataDir data_dir, DWORD data_directory_size)
  {
    if (static_cast<DWORD>(data_dir) >= GetNumberOfRvaAndSizes())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                      << ErrorString{"Invalid data dir."});
    }

    if (pe_file_->Is64())
    {
      data_64_.OptionalHeader
        .DataDirectory[static_cast<std::uint32_t>(data_dir)]
        .Size = data_directory_size;
    }
    else
    {
      data_32_.OptionalHeader
        .DataDirectory[static_cast<std::uint32_t>(data_dir)]
        .Size = data_directory_size;
    }
  }

private:
  PBYTE CalculateBase(Process const& process, PeFile const& pe_file) const
  {
    DosHeader dos_header{process, pe_file};
    return static_cast<PBYTE>(dos_header.GetBase()) +
           dos_header.GetNewHeaderOffset();
  }

  Process const* process_;
  PeFile const* pe_file_;
  std::uint8_t* base_;
  IMAGE_NT_HEADERS32 data_32_ = IMAGE_NT_HEADERS32{};
  IMAGE_NT_HEADERS64 data_64_ = IMAGE_NT_HEADERS64{};
};

inline bool operator==(NtHeaders const& lhs, NtHeaders const& rhs) noexcept
{
  return lhs.GetBase() == rhs.GetBase();
}

inline bool operator!=(NtHeaders const& lhs, NtHeaders const& rhs) noexcept
{
  return !(lhs == rhs);
}

inline bool operator<(NtHeaders const& lhs, NtHeaders const& rhs) noexcept
{
  return lhs.GetBase() < rhs.GetBase();
}

inline bool operator<=(NtHeaders const& lhs, NtHeaders const& rhs) noexcept
{
  return lhs.GetBase() <= rhs.GetBase();
}

inline bool operator>(NtHeaders const& lhs, NtHeaders const& rhs) noexcept
{
  return lhs.GetBase() > rhs.GetBase();
}

inline bool operator>=(NtHeaders const& lhs, NtHeaders const& rhs) noexcept
{
  return lhs.GetBase() >= rhs.GetBase();
}

inline std::ostream& operator<<(std::ostream& lhs, NtHeaders const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << static_cast<void*>(rhs.GetBase());
  lhs.imbue(old);
  return lhs;
}

inline std::wostream& operator<<(std::wostream& lhs, NtHeaders const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << static_cast<void*>(rhs.GetBase());
  lhs.imbue(old);
  return lhs;
}

inline ULONGLONG GetRuntimeBase(Process const& process, PeFile const& pe_file)
{
  switch (pe_file.GetType())
  {
  case PeFileType::kImage:
    return reinterpret_cast<ULONGLONG>(pe_file.GetBase());
  case PeFileType::kData:
    return NtHeaders(process, pe_file).GetImageBase();
  }

  HADESMEM_DETAIL_ASSERT(false);
  return 0;
}
}

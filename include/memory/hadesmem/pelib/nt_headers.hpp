// Copyright (C) 2010-2014 Joshua Boyce.
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

  explicit NtHeaders(Process&& process, PeFile const& pe_file) = delete;

  explicit NtHeaders(Process const& process, PeFile&& pe_file) = delete;

  explicit NtHeaders(Process&& process, PeFile&& pe_file) = delete;

  PVOID GetBase() const HADESMEM_DETAIL_NOEXCEPT
  {
    return base_;
  }

  void UpdateRead()
  {
    data_ = Read<IMAGE_NT_HEADERS>(*process_, base_);
  }

  void UpdateWrite()
  {
    Write(*process_, base_, data_);
  }

  bool IsValid() const
  {
    return IMAGE_NT_SIGNATURE == GetSignature() &&
           IMAGE_NT_OPTIONAL_HDR_MAGIC == GetMagic() &&
#if defined(HADESMEM_DETAIL_ARCH_X86)
           IMAGE_FILE_MACHINE_I386 == GetMachine();
#elif defined(HADESMEM_DETAIL_ARCH_X64)
           IMAGE_FILE_MACHINE_AMD64 == GetMachine();
#else
#error "[HadesMem] Unsupported architecture."
#endif
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
    return data_.Signature;
  }

  WORD GetMachine() const
  {
    return data_.FileHeader.Machine;
  }

  WORD GetNumberOfSections() const
  {
    return data_.FileHeader.NumberOfSections;
  }

  DWORD GetTimeDateStamp() const
  {
    return data_.FileHeader.TimeDateStamp;
  }

  DWORD GetPointerToSymbolTable() const
  {
    return data_.FileHeader.PointerToSymbolTable;
  }

  DWORD GetNumberOfSymbols() const
  {
    return data_.FileHeader.NumberOfSymbols;
  }

  WORD GetSizeOfOptionalHeader() const
  {
    return data_.FileHeader.SizeOfOptionalHeader;
  }

  WORD GetCharacteristics() const
  {
    return data_.FileHeader.Characteristics;
  }

  WORD GetMagic() const
  {
    return data_.OptionalHeader.Magic;
  }

  BYTE GetMajorLinkerVersion() const
  {
    return data_.OptionalHeader.MajorLinkerVersion;
  }

  BYTE GetMinorLinkerVersion() const
  {
    return data_.OptionalHeader.MinorLinkerVersion;
  }

  DWORD GetSizeOfCode() const
  {
    return data_.OptionalHeader.SizeOfCode;
  }

  DWORD GetSizeOfInitializedData() const
  {
    return data_.OptionalHeader.SizeOfInitializedData;
  }

  DWORD GetSizeOfUninitializedData() const
  {
    return data_.OptionalHeader.SizeOfUninitializedData;
  }

  DWORD GetAddressOfEntryPoint() const
  {
    return data_.OptionalHeader.AddressOfEntryPoint;
  }

  DWORD GetBaseOfCode() const
  {
    return data_.OptionalHeader.BaseOfCode;
  }

#if defined(HADESMEM_DETAIL_ARCH_X86)
  DWORD GetBaseOfData() const
  {
    return data_.OptionalHeader.BaseOfData;
  }
#endif

  ULONG_PTR GetImageBase() const
  {
    return data_.OptionalHeader.ImageBase;
  }

  DWORD GetSectionAlignment() const
  {
    return data_.OptionalHeader.SectionAlignment;
  }

  DWORD GetFileAlignment() const
  {
    return data_.OptionalHeader.FileAlignment;
  }

  WORD GetMajorOperatingSystemVersion() const
  {
    return data_.OptionalHeader.MajorOperatingSystemVersion;
  }

  WORD GetMinorOperatingSystemVersion() const
  {
    return data_.OptionalHeader.MinorOperatingSystemVersion;
  }

  WORD GetMajorImageVersion() const
  {
    return data_.OptionalHeader.MajorImageVersion;
  }

  WORD GetMinorImageVersion() const
  {
    return data_.OptionalHeader.MinorImageVersion;
  }

  WORD GetMajorSubsystemVersion() const
  {
    return data_.OptionalHeader.MajorSubsystemVersion;
  }

  WORD GetMinorSubsystemVersion() const
  {
    return data_.OptionalHeader.MinorSubsystemVersion;
  }

  DWORD GetWin32VersionValue() const
  {
    return data_.OptionalHeader.Win32VersionValue;
  }

  DWORD GetSizeOfImage() const
  {
    return data_.OptionalHeader.SizeOfImage;
  }

  DWORD GetSizeOfHeaders() const
  {
    return data_.OptionalHeader.SizeOfHeaders;
  }

  DWORD GetCheckSum() const
  {
    return data_.OptionalHeader.CheckSum;
  }

  WORD GetSubsystem() const
  {
    return data_.OptionalHeader.Subsystem;
  }

  WORD GetDllCharacteristics() const
  {
    return data_.OptionalHeader.DllCharacteristics;
  }

  ULONG_PTR GetSizeOfStackReserve() const
  {
    return data_.OptionalHeader.SizeOfStackReserve;
  }

  ULONG_PTR GetSizeOfStackCommit() const
  {
    return data_.OptionalHeader.SizeOfStackCommit;
  }

  ULONG_PTR GetSizeOfHeapReserve() const
  {
    return data_.OptionalHeader.SizeOfHeapReserve;
  }

  ULONG_PTR GetSizeOfHeapCommit() const
  {
    return data_.OptionalHeader.SizeOfHeapCommit;
  }

  DWORD GetLoaderFlags() const
  {
    return data_.OptionalHeader.LoaderFlags;
  }

  DWORD GetNumberOfRvaAndSizes() const
  {
    return data_.OptionalHeader.NumberOfRvaAndSizes;
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

    return data_.OptionalHeader.DataDirectory[static_cast<std::uint32_t>(
                                                data_dir)].VirtualAddress;
  }

  DWORD GetDataDirectorySize(PeDataDir data_dir) const
  {
    if (static_cast<DWORD>(data_dir) >= GetNumberOfRvaAndSizesClamped())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                      << ErrorString{"Invalid data dir."});
    }

    return data_.OptionalHeader.DataDirectory[static_cast<std::uint32_t>(
                                                data_dir)].Size;
  }

  void SetSignature(DWORD signature)
  {
    data_.Signature = signature;
  }

  void SetMachine(WORD machine)
  {
    data_.FileHeader.Machine = machine;
  }

  void SetNumberOfSections(WORD number_of_sections)
  {
    data_.FileHeader.NumberOfSections = number_of_sections;
  }

  void SetTimeDateStamp(DWORD time_date_stamp)
  {
    data_.FileHeader.TimeDateStamp = time_date_stamp;
  }

  void SetPointerToSymbolTable(DWORD pointer_to_symbol_table)
  {
    data_.FileHeader.PointerToSymbolTable = pointer_to_symbol_table;
  }

  void SetNumberOfSymbols(DWORD number_of_symbols)
  {
    data_.FileHeader.NumberOfSymbols = number_of_symbols;
  }

  void SetSizeOfOptionalHeader(WORD size_of_optional_header)
  {
    data_.FileHeader.SizeOfOptionalHeader = size_of_optional_header;
  }

  void SetCharacteristics(WORD characteristics)
  {
    data_.FileHeader.Characteristics = characteristics;
  }

  void SetMagic(WORD magic)
  {
    data_.OptionalHeader.Magic = magic;
  }

  void SetMajorLinkerVersion(BYTE major_linker_version)
  {
    data_.OptionalHeader.MajorLinkerVersion = major_linker_version;
  }

  void SetMinorLinkerVersion(BYTE minor_linker_version)
  {
    data_.OptionalHeader.MinorLinkerVersion = minor_linker_version;
  }

  void SetSizeOfCode(DWORD size_of_code)
  {
    data_.OptionalHeader.SizeOfCode = size_of_code;
  }

  void SetSizeOfInitializedData(DWORD size_of_initialized_data)
  {
    data_.OptionalHeader.SizeOfInitializedData = size_of_initialized_data;
  }

  void SetSizeOfUninitializedData(DWORD size_of_uninitialized_data)
  {
    data_.OptionalHeader.SizeOfUninitializedData = size_of_uninitialized_data;
  }

  void SetAddressOfEntryPoint(DWORD address_of_entry_point)
  {
    data_.OptionalHeader.AddressOfEntryPoint = address_of_entry_point;
  }

  void SetBaseOfCode(DWORD base_of_code)
  {
    data_.OptionalHeader.BaseOfCode = base_of_code;
  }

#if defined(HADESMEM_DETAIL_ARCH_X86)
  void SetBaseOfData(DWORD base_of_data)
  {
    data_.OptionalHeader.BaseOfData = base_of_data;
  }
#endif

  void SetImageBase(ULONG_PTR image_base)
  {
    data_.OptionalHeader.ImageBase = image_base;
  }

  void SetSectionAlignment(DWORD section_alignment)
  {
    data_.OptionalHeader.SectionAlignment = section_alignment;
  }

  void SetFileAlignment(DWORD file_alignment)
  {
    data_.OptionalHeader.FileAlignment = file_alignment;
  }

  void SetMajorOperatingSystemVersion(WORD major_operating_system_version)
  {
    data_.OptionalHeader.MajorOperatingSystemVersion =
      major_operating_system_version;
  }

  void SetMinorOperatingSystemVersion(WORD minor_operating_system_version)
  {
    data_.OptionalHeader.MinorOperatingSystemVersion =
      minor_operating_system_version;
  }

  void SetMajorImageVersion(WORD major_image_version)
  {
    data_.OptionalHeader.MajorImageVersion = major_image_version;
  }

  void SetMinorImageVersion(WORD minor_image_version)
  {
    data_.OptionalHeader.MinorImageVersion = minor_image_version;
  }

  void SetMajorSubsystemVersion(WORD major_subsystem_version)
  {
    data_.OptionalHeader.MajorSubsystemVersion = major_subsystem_version;
  }

  void SetMinorSubsystemVersion(WORD minor_subsystem_version)
  {
    data_.OptionalHeader.MinorSubsystemVersion = minor_subsystem_version;
  }

  void SetWin32VersionValue(DWORD win32_version_value)
  {
    data_.OptionalHeader.Win32VersionValue = win32_version_value;
  }

  void SetSizeOfImage(DWORD size_of_image)
  {
    data_.OptionalHeader.SizeOfImage = size_of_image;
  }

  void SetSizeOfHeaders(DWORD size_of_headers)
  {
    data_.OptionalHeader.SizeOfHeaders = size_of_headers;
  }

  void SetCheckSum(DWORD checksum)
  {
    data_.OptionalHeader.CheckSum = checksum;
  }

  void SetSubsystem(WORD subsystem)
  {
    data_.OptionalHeader.Subsystem = subsystem;
  }

  void SetDllCharacteristics(WORD dll_characteristics)
  {
    data_.OptionalHeader.DllCharacteristics = dll_characteristics;
  }

  void SetSizeOfStackReserve(ULONG_PTR size_of_stack_reserve)
  {
    data_.OptionalHeader.SizeOfStackReserve = size_of_stack_reserve;
  }

  void SetSizeOfStackCommit(ULONG_PTR size_of_stack_commit)
  {
    data_.OptionalHeader.SizeOfStackCommit = size_of_stack_commit;
  }

  void SetSizeOfHeapReserve(ULONG_PTR size_of_heap_reserve)
  {
    data_.OptionalHeader.SizeOfHeapReserve = size_of_heap_reserve;
  }

  void SetSizeOfHeapCommit(ULONG_PTR size_of_heap_commit)
  {
    data_.OptionalHeader.SizeOfHeapCommit = size_of_heap_commit;
  }

  void SetLoaderFlags(DWORD loader_flags)
  {
    data_.OptionalHeader.LoaderFlags = loader_flags;
  }

  void SetNumberOfRvaAndSizes(DWORD number_of_rva_and_sizes)
  {
    data_.OptionalHeader.NumberOfRvaAndSizes = number_of_rva_and_sizes;
  }

  void SetDataDirectoryVirtualAddress(PeDataDir data_dir,
                                      DWORD data_directory_virtual_address)
  {
    if (static_cast<DWORD>(data_dir) >= GetNumberOfRvaAndSizes())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                      << ErrorString{"Invalid data dir."});
    }

    data_.OptionalHeader.DataDirectory[static_cast<std::uint32_t>(data_dir)]
      .VirtualAddress = data_directory_virtual_address;
  }

  void SetDataDirectorySize(PeDataDir data_dir, DWORD data_directory_size)
  {
    if (static_cast<DWORD>(data_dir) >= GetNumberOfRvaAndSizes())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                      << ErrorString{"Invalid data dir."});
    }

    data_.OptionalHeader.DataDirectory[static_cast<std::uint32_t>(data_dir)]
      .Size = data_directory_size;
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
  IMAGE_NT_HEADERS data_ = IMAGE_NT_HEADERS{};
};

inline bool operator==(NtHeaders const& lhs,
                       NtHeaders const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

inline bool operator!=(NtHeaders const& lhs,
                       NtHeaders const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(NtHeaders const& lhs,
                      NtHeaders const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

inline bool operator<=(NtHeaders const& lhs,
                       NtHeaders const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

inline bool operator>(NtHeaders const& lhs,
                      NtHeaders const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

inline bool operator>=(NtHeaders const& lhs,
                       NtHeaders const& rhs) HADESMEM_DETAIL_NOEXCEPT
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

inline ULONG_PTR GetRuntimeBase(Process const& process, PeFile const& pe_file)
{
  switch (pe_file.GetType())
  {
  case PeFileType::Image:
    return reinterpret_cast<ULONG_PTR>(pe_file.GetBase());
  case PeFileType::Data:
    return NtHeaders(process, pe_file).GetImageBase();
  }

  HADESMEM_DETAIL_ASSERT(false);
  return 0;
}
}

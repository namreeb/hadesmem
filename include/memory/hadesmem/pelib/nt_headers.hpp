// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <array>
#include <iosfwd>
#include <memory>
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
#include <hadesmem/pelib/dos_header.hpp>

#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextended-offsetof"
#endif

namespace hadesmem
{

class Process;

class PeFile;

enum class PeDataDir : DWORD
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
  COMDescriptor
};

class NtHeaders
{
public:
  explicit NtHeaders(Process const& process, PeFile const& pe_file)
    : process_(&process), 
    pe_file_(&pe_file), 
    base_(CalculateBase(*process_, *pe_file_))
  {
    EnsureValid();
  }

  NtHeaders(NtHeaders const& other)
    : process_(other.process_), 
    pe_file_(other.pe_file_), 
    base_(other.base_)
  { }
  
  NtHeaders& operator=(NtHeaders const& other)
  {
    process_ = other.process_;
    pe_file_ = other.pe_file_;
    base_ = other.base_;

    return *this;
  }

  NtHeaders(NtHeaders&& other) HADESMEM_NOEXCEPT
    : process_(other.process_), 
    pe_file_(other.pe_file_), 
    base_(other.base_)
  { }
  
  NtHeaders& operator=(NtHeaders&& other) HADESMEM_NOEXCEPT
  {
    process_ = other.process_;
    pe_file_ = other.pe_file_;
    base_ = other.base_;

    return *this;
  }
  
  ~NtHeaders() HADESMEM_NOEXCEPT
  { }
  
  PVOID GetBase() const HADESMEM_NOEXCEPT
  {
    return base_;
  }

  bool IsValid() const
  {
    // TODO: Check whether the Magic check should be removed (i.e. whether 
    // Windows will load images with a NULL or invalid value).
    return IMAGE_NT_SIGNATURE == GetSignature() && 
      IMAGE_NT_OPTIONAL_HDR_MAGIC == GetMagic();
  }

  void EnsureValid() const
  {
    if (!IsValid())
    {
      HADESMEM_THROW_EXCEPTION(Error() << 
        ErrorString("NT headers signature invalid."));
    }
  }

  DWORD GetSignature() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      Signature));
  }

  WORD GetMachine() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      FileHeader.Machine));
  }

  WORD GetNumberOfSections() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      FileHeader.NumberOfSections));
  }

  DWORD GetTimeDateStamp() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      FileHeader.TimeDateStamp));
  }

  DWORD GetPointerToSymbolTable() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      FileHeader.PointerToSymbolTable));
  }

  DWORD GetNumberOfSymbols() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      FileHeader.NumberOfSymbols));
  }

  WORD GetSizeOfOptionalHeader() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      FileHeader.SizeOfOptionalHeader));
  }

  WORD GetCharacteristics() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      FileHeader.Characteristics));
  }

  WORD GetMagic() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.Magic));
  }

  BYTE GetMajorLinkerVersion() const
  {
    return Read<BYTE>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.MajorLinkerVersion));
  }

  BYTE GetMinorLinkerVersion() const
  {
    return Read<BYTE>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.MinorLinkerVersion));
  }

  DWORD GetSizeOfCode() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.SizeOfCode));
  }

  DWORD GetSizeOfInitializedData() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.SizeOfInitializedData));
  }

  DWORD GetSizeOfUninitializedData() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.SizeOfUninitializedData));
  }

  DWORD GetAddressOfEntryPoint() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.AddressOfEntryPoint));
  }

  DWORD GetBaseOfCode() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.BaseOfCode));
  }

#if defined(_M_IX86) 
  DWORD GetBaseOfData() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.BaseOfData));
  }
#endif

  ULONG_PTR GetImageBase() const
  {
    return Read<ULONG_PTR>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.ImageBase));
  }

  DWORD GetSectionAlignment() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.SectionAlignment));
  }

  DWORD GetFileAlignment() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.FileAlignment));
  }

  WORD GetMajorOperatingSystemVersion() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.MajorOperatingSystemVersion));
  }

  WORD GetMinorOperatingSystemVersion() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.MinorOperatingSystemVersion));
  }

  WORD GetMajorImageVersion() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.MajorImageVersion));
  }

  WORD GetMinorImageVersion() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.MinorImageVersion));
  }

  WORD GetMajorSubsystemVersion() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.MajorSubsystemVersion));
  }

  WORD GetMinorSubsystemVersion() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.MinorSubsystemVersion));
  }

  DWORD GetWin32VersionValue() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.Win32VersionValue));
  }

  DWORD GetSizeOfImage() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.SizeOfImage));
  }

  DWORD GetSizeOfHeaders() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.SizeOfHeaders));
  }

  DWORD GetCheckSum() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.CheckSum));
  }

  WORD GetSubsystem() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.Subsystem));
  }

  WORD GetDllCharacteristics() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.DllCharacteristics));
  }

  ULONG_PTR GetSizeOfStackReserve() const
  {
    return Read<ULONG_PTR>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.SizeOfStackReserve));
  }

  ULONG_PTR GetSizeOfStackCommit() const
  {
    return Read<ULONG_PTR>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.SizeOfStackCommit));
  }

  ULONG_PTR GetSizeOfHeapReserve() const
  {
    return Read<ULONG_PTR>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.SizeOfHeapReserve));
  }

  ULONG_PTR GetSizeOfHeapCommit() const
  {
    return Read<ULONG_PTR>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.SizeOfHeapCommit));
  }

  DWORD GetLoaderFlags() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.LoaderFlags));
  }

  DWORD GetNumberOfRvaAndSizes() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.NumberOfRvaAndSizes));
  }

  DWORD GetDataDirectoryVirtualAddress(PeDataDir data_dir) const
  {
    if (static_cast<DWORD>(data_dir) >= GetNumberOfRvaAndSizes())
    {
      return 0;
    }

    return Read<DWORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.DataDirectory[0]) + static_cast<DWORD>(data_dir) * 
      sizeof(IMAGE_DATA_DIRECTORY) + offsetof(IMAGE_DATA_DIRECTORY, 
      VirtualAddress));
  }

  DWORD GetDataDirectorySize(PeDataDir data_dir) const
  {
    if (static_cast<DWORD>(data_dir) >= GetNumberOfRvaAndSizes())
    {
      return 0;
    }

    return Read<DWORD>(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.DataDirectory[0]) + static_cast<DWORD>(data_dir) * 
      sizeof(IMAGE_DATA_DIRECTORY) + offsetof(IMAGE_DATA_DIRECTORY, Size));
  }
  
  void SetSignature(DWORD signature)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      Signature), signature);
  }

  void SetMachine(WORD machine)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      FileHeader.Machine), machine);
  }

  void SetNumberOfSections(WORD number_of_sections)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      FileHeader.NumberOfSections), number_of_sections);
  }

  void SetTimeDateStamp(DWORD time_date_stamp)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      FileHeader.TimeDateStamp), time_date_stamp);
  }

  void SetPointerToSymbolTable(DWORD pointer_to_symbol_table)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      FileHeader.PointerToSymbolTable), pointer_to_symbol_table);
  }

  void SetNumberOfSymbols(DWORD number_of_symbols)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      FileHeader.NumberOfSymbols), number_of_symbols);
  }

  void SetSizeOfOptionalHeader(WORD size_of_optional_header)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      FileHeader.SizeOfOptionalHeader), size_of_optional_header);

  }

  void SetCharacteristics(WORD characteristics)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      FileHeader.Characteristics), characteristics);
  }

  void SetMagic(WORD magic)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.Magic), magic);
  }

  void SetMajorLinkerVersion(BYTE major_linker_version)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.MajorLinkerVersion), major_linker_version);
  }

  void SetMinorLinkerVersion(BYTE minor_linker_version)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.MinorLinkerVersion), minor_linker_version);
  }

  void SetSizeOfCode(DWORD size_of_code)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.SizeOfCode), size_of_code);
  }

  void SetSizeOfInitializedData(DWORD size_of_initialized_data)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.SizeOfInitializedData), size_of_initialized_data);
  }

  void SetSizeOfUninitializedData(DWORD size_of_uninitialized_data)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.SizeOfUninitializedData), size_of_uninitialized_data);
  }

  void SetAddressOfEntryPoint(DWORD address_of_entry_point)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.AddressOfEntryPoint), address_of_entry_point);
  }

  void SetBaseOfCode(DWORD base_of_code)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.BaseOfCode), base_of_code);
  }

  #if defined(_M_IX86) 
  void SetBaseOfData(DWORD base_of_data)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.BaseOfData), base_of_data);
  }
  #endif

  void SetImageBase(ULONG_PTR image_base)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.ImageBase), image_base);
  }

  void SetSectionAlignment(DWORD section_alignment)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.SectionAlignment), section_alignment);
  }

  void SetFileAlignment(DWORD file_alignment)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.FileAlignment), file_alignment);
  }

  void SetMajorOperatingSystemVersion(
    WORD major_operating_system_version)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.MajorOperatingSystemVersion), 
      major_operating_system_version);
  }

  void SetMinorOperatingSystemVersion(
    WORD minor_operating_system_version)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.MinorOperatingSystemVersion), 
      minor_operating_system_version);
  }

  void SetMajorImageVersion(WORD major_image_version)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.MajorImageVersion), major_image_version);
  }

  void SetMinorImageVersion(WORD minor_image_version)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.MinorImageVersion), minor_image_version);
  }

  void SetMajorSubsystemVersion(WORD major_subsystem_version)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.MajorSubsystemVersion), major_subsystem_version);
  }

  void SetMinorSubsystemVersion(WORD minor_subsystem_version)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.MinorSubsystemVersion), minor_subsystem_version);
  }

  void SetWin32VersionValue(DWORD win32_version_value)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.Win32VersionValue), win32_version_value);
  }

  void SetSizeOfImage(DWORD size_of_image)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.SizeOfImage), size_of_image);
  }

  void SetSizeOfHeaders(DWORD size_of_headers)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.SizeOfHeaders), size_of_headers);
  }

  void SetCheckSum(DWORD checksum)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.CheckSum), checksum);
  }

  void SetSubsystem(WORD subsystem)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.Subsystem), subsystem);
  }

  void SetDllCharacteristics(WORD dll_characteristics)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.DllCharacteristics), dll_characteristics);
  }

  void SetSizeOfStackReserve(ULONG_PTR size_of_stack_reserve)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.SizeOfStackReserve), size_of_stack_reserve);
  }

  void SetSizeOfStackCommit(ULONG_PTR size_of_stack_commit)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.SizeOfStackCommit), size_of_stack_commit);
  }

  void SetSizeOfHeapReserve(ULONG_PTR size_of_heap_reserve)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.SizeOfHeapReserve), size_of_heap_reserve);
  }

  void SetSizeOfHeapCommit(ULONG_PTR size_of_heap_commit)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.SizeOfHeapCommit), size_of_heap_commit);
  }

  void SetLoaderFlags(DWORD loader_flags)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.LoaderFlags), loader_flags);
  }

  void SetNumberOfRvaAndSizes(DWORD number_of_rva_and_sizes)
  {
    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.NumberOfRvaAndSizes), number_of_rva_and_sizes);
  }

  void SetDataDirectoryVirtualAddress(PeDataDir data_dir, 
    DWORD data_directory_virtual_address)
  {
    if (static_cast<DWORD>(data_dir) >= GetNumberOfRvaAndSizes())
    {
      return;
    }

    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.DataDirectory[0]) + static_cast<DWORD>(data_dir) * 
      sizeof(IMAGE_DATA_DIRECTORY) + offsetof(IMAGE_DATA_DIRECTORY, 
      VirtualAddress), data_directory_virtual_address);
  }

  void SetDataDirectorySize(PeDataDir data_dir, 
    DWORD data_directory_size)
  {
    if (static_cast<DWORD>(data_dir) >= GetNumberOfRvaAndSizes())
    {
      return;
    }

    Write(*process_, base_ + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader.DataDirectory[0]) + static_cast<DWORD>(data_dir) * 
      sizeof(IMAGE_DATA_DIRECTORY) + offsetof(IMAGE_DATA_DIRECTORY, Size), 
      data_directory_size);
  }

private:
  PBYTE CalculateBase(Process const& process, PeFile const& pe_file) const
  {
    DosHeader dos_header(process, pe_file);
    return static_cast<PBYTE>(dos_header.GetBase()) + 
      dos_header.GetNewHeaderOffset();
  }

  Process const* process_;
  PeFile const* pe_file_;
  PBYTE base_;
};

inline bool operator==(NtHeaders const& lhs, NtHeaders const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

inline bool operator!=(NtHeaders const& lhs, NtHeaders const& rhs) HADESMEM_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(NtHeaders const& lhs, NtHeaders const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

inline bool operator<=(NtHeaders const& lhs, NtHeaders const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

inline bool operator>(NtHeaders const& lhs, NtHeaders const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

inline bool operator>=(NtHeaders const& lhs, NtHeaders const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

inline std::ostream& operator<<(std::ostream& lhs, NtHeaders const& rhs)
{
  return (lhs << rhs.GetBase());
}

inline std::wostream& operator<<(std::wostream& lhs, NtHeaders const& rhs)
{
  return (lhs << rhs.GetBase());
}

}

#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic pop
#endif

// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <array>
#include <iosfwd>
#include <memory>

#include <windows.h>

#include "hadesmem/config.hpp"

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
  explicit NtHeaders(Process const& process, PeFile const& pe_file);

  NtHeaders(NtHeaders const& other);
  
  NtHeaders& operator=(NtHeaders const& other);

  NtHeaders(NtHeaders&& other) HADESMEM_NOEXCEPT;
  
  NtHeaders& operator=(NtHeaders&& other) HADESMEM_NOEXCEPT;
  
  ~NtHeaders();

  PVOID GetBase() const HADESMEM_NOEXCEPT;

  bool IsValid() const;

  void EnsureValid() const;

  DWORD GetSignature() const;

  WORD GetMachine() const;

  WORD GetNumberOfSections() const;

  DWORD GetTimeDateStamp() const;

  DWORD GetPointerToSymbolTable() const;

  DWORD GetNumberOfSymbols() const;

  WORD GetSizeOfOptionalHeader() const;

  WORD GetCharacteristics() const;

  WORD GetMagic() const;

  BYTE GetMajorLinkerVersion() const;

  BYTE GetMinorLinkerVersion() const;

  DWORD GetSizeOfCode() const;

  DWORD GetSizeOfInitializedData() const;

  DWORD GetSizeOfUninitializedData() const;

  DWORD GetAddressOfEntryPoint() const;

  DWORD GetBaseOfCode() const;

#if defined(HADESMEM_ARCH_X86) 
  DWORD GetBaseOfData() const;
#endif

  ULONG_PTR GetImageBase() const;

  DWORD GetSectionAlignment() const;

  DWORD GetFileAlignment() const;

  WORD GetMajorOperatingSystemVersion() const;

  WORD GetMinorOperatingSystemVersion() const;

  WORD GetMajorImageVersion() const;

  WORD GetMinorImageVersion() const;

  WORD GetMajorSubsystemVersion() const;

  WORD GetMinorSubsystemVersion() const;

  DWORD GetWin32VersionValue() const;

  DWORD GetSizeOfImage() const;

  DWORD GetSizeOfHeaders() const;

  DWORD GetCheckSum() const;

  WORD GetSubsystem() const;

  WORD GetDllCharacteristics() const;

  ULONG_PTR GetSizeOfStackReserve() const;

  ULONG_PTR GetSizeOfStackCommit() const;

  ULONG_PTR GetSizeOfHeapReserve() const;

  ULONG_PTR GetSizeOfHeapCommit() const;

  DWORD GetLoaderFlags() const;

  DWORD GetNumberOfRvaAndSizes() const;

  DWORD GetDataDirectoryVirtualAddress(PeDataDir data_dir) const;

  DWORD GetDataDirectorySize(PeDataDir data_dir) const;

  void SetSignature(DWORD signature);

  void SetMachine(WORD machine);

  void SetNumberOfSections(WORD number_of_sections);

  void SetTimeDateStamp(DWORD time_date_stamp);

  void SetPointerToSymbolTable(DWORD pointer_to_symbol_table);

  void SetNumberOfSymbols(DWORD number_of_symbols);

  void SetSizeOfOptionalHeader(WORD size_of_optional_header);

  void SetCharacteristics(WORD characteristics);

  void SetMagic(WORD magic);

  void SetMajorLinkerVersion(BYTE major_linker_version);

  void SetMinorLinkerVersion(BYTE minor_linker_version);

  void SetSizeOfCode(DWORD size_of_code);

  void SetSizeOfInitializedData(DWORD size_of_initialized_data);

  void SetSizeOfUninitializedData(DWORD size_of_uninitialized_data);

  void SetAddressOfEntryPoint(DWORD address_of_entry_point);

  void SetBaseOfCode(DWORD base_of_code);

#if defined(HADESMEM_ARCH_X86) 
  void SetBaseOfData(DWORD base_of_data);
#endif

  void SetImageBase(ULONG_PTR image_base);

  void SetSectionAlignment(DWORD section_alignment);

  void SetFileAlignment(DWORD file_alignment);

  void SetMajorOperatingSystemVersion(
    WORD major_operating_system_version);

  void SetMinorOperatingSystemVersion(
    WORD minor_operating_system_version);

  void SetMajorImageVersion(WORD major_image_version);

  void SetMinorImageVersion(WORD minor_image_version);

  void SetMajorSubsystemVersion(WORD major_subsystem_version);

  void SetMinorSubsystemVersion(WORD minor_subsystem_version);

  void SetWin32VersionValue(DWORD win32_version_value);

  void SetSizeOfImage(DWORD size_of_image);

  void SetSizeOfHeaders(DWORD size_of_headers);

  void SetCheckSum(DWORD checksum);

  void SetSubsystem(WORD subsystem);

  void SetDllCharacteristics(WORD dll_characteristics);

  void SetSizeOfStackReserve(ULONG_PTR size_of_stack_reserve);

  void SetSizeOfStackCommit(ULONG_PTR size_of_stack_commit);

  void SetSizeOfHeapReserve(ULONG_PTR size_of_heap_reserve);

  void SetSizeOfHeapCommit(ULONG_PTR size_of_heap_commit);

  void SetLoaderFlags(DWORD loader_flags);

  void SetNumberOfRvaAndSizes(DWORD number_of_rva_and_sizes);

  void SetDataDirectoryVirtualAddress(PeDataDir data_dir, 
    DWORD data_directory_virtual_address);

  void SetDataDirectorySize(PeDataDir data_dir, 
    DWORD data_directory_size);
  
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

bool operator==(NtHeaders const& lhs, NtHeaders const& rhs) HADESMEM_NOEXCEPT;

bool operator!=(NtHeaders const& lhs, NtHeaders const& rhs) HADESMEM_NOEXCEPT;

bool operator<(NtHeaders const& lhs, NtHeaders const& rhs) HADESMEM_NOEXCEPT;

bool operator<=(NtHeaders const& lhs, NtHeaders const& rhs) HADESMEM_NOEXCEPT;

bool operator>(NtHeaders const& lhs, NtHeaders const& rhs) HADESMEM_NOEXCEPT;

bool operator>=(NtHeaders const& lhs, NtHeaders const& rhs) HADESMEM_NOEXCEPT;

std::ostream& operator<<(std::ostream& lhs, NtHeaders const& rhs);

std::wostream& operator<<(std::wostream& lhs, NtHeaders const& rhs);

}

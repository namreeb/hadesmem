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

enum class PeDataDir : unsigned int
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

#if defined(_M_IX86) 
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

  void SetSignature(DWORD signature) const;

  void SetMachine(WORD machine) const;

  void SetNumberOfSections(WORD number_of_sections) const;

  void SetTimeDateStamp(DWORD time_date_stamp) const;

  void SetPointerToSymbolTable(DWORD pointer_to_symbol_table) const;

  void SetNumberOfSymbols(DWORD number_of_symbols) const;

  void SetSizeOfOptionalHeader(WORD size_of_optional_header) const;

  void SetCharacteristics(WORD characteristics) const;

  void SetMagic(WORD magic) const;

  void SetMajorLinkerVersion(BYTE major_linker_version) const;

  void SetMinorLinkerVersion(BYTE minor_linker_version) const;

  void SetSizeOfCode(DWORD size_of_code) const;

  void SetSizeOfInitializedData(DWORD size_of_initialized_data) const;

  void SetSizeOfUninitializedData(DWORD size_of_uninitialized_data) const;

  void SetAddressOfEntryPoint(DWORD address_of_entry_point) const;

  void SetBaseOfCode(DWORD base_of_code) const;

#if defined(_M_IX86) 
  void SetBaseOfData(DWORD base_of_data) const;
#endif

  void SetImageBase(ULONG_PTR image_base) const;

  void SetSectionAlignment(DWORD section_alignment) const;

  void SetFileAlignment(DWORD file_alignment) const;

  void SetMajorOperatingSystemVersion(
    WORD major_operating_system_version) const;

  void SetMinorOperatingSystemVersion(
    WORD minor_operating_system_version) const;

  void SetMajorImageVersion(WORD major_image_version) const;

  void SetMinorImageVersion(WORD minor_image_version) const;

  void SetMajorSubsystemVersion(WORD major_subsystem_version) const;

  void SetMinorSubsystemVersion(WORD minor_subsystem_version) const;

  void SetWin32VersionValue(DWORD win32_version_value) const;

  void SetSizeOfImage(DWORD size_of_image) const;

  void SetSizeOfHeaders(DWORD size_of_headers) const;

  void SetCheckSum(DWORD checksum) const;

  void SetSubsystem(WORD subsystem) const;

  void SetDllCharacteristics(WORD dll_characteristics) const;

  void SetSizeOfStackReserve(ULONG_PTR size_of_stack_reserve) const;

  void SetSizeOfStackCommit(ULONG_PTR size_of_stack_commit) const;

  void SetSizeOfHeapReserve(ULONG_PTR size_of_heap_reserve) const;

  void SetSizeOfHeapCommit(ULONG_PTR size_of_heap_commit) const;

  void SetLoaderFlags(DWORD loader_flags) const;

  void SetNumberOfRvaAndSizes(DWORD number_of_rva_and_sizes) const;

  void SetDataDirectoryVirtualAddress(PeDataDir data_dir, 
    DWORD data_directory_virtual_address) const;

  void SetDataDirectorySize(PeDataDir data_dir, 
    DWORD data_directory_size) const;
  
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

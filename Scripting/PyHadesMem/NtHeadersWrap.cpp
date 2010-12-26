/*
This file is part of HadesMem.
Copyright © 2010 RaptorFactor (aka Cypherjb, Cypher, Chazwazza). 
<http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

HadesMem is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

HadesMem is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with HadesMem.  If not, see <http://www.gnu.org/licenses/>.
*/

// Boost
#ifdef _MSC_VER
#pragma warning(push, 1)
#endif // #ifdef _MSC_VER
#include <boost/python.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif // #ifdef _MSC_VER

// Hades
#include "Memory/NtHeaders.h"

class NtHeadersWrap : public Hades::Memory::NtHeaders
{
public:
  explicit NtHeadersWrap(Hades::Memory::PeFile const& MyPeFile)
    : Hades::Memory::NtHeaders(MyPeFile)
  { }

  DWORD_PTR GetBase() const
  {
    return reinterpret_cast<DWORD_PTR>(Hades::Memory::NtHeaders::GetBase());
  }
};

// Export NtHeaders API
void ExportNtHeaders()
{
  boost::python::class_<Hades::Memory::NtHeaders>("NtHeadersBase", 
    boost::python::no_init)
    ;

  boost::python::scope NtHeadersScope = boost::python::class_<NtHeadersWrap, 
    boost::python::bases<Hades::Memory::NtHeaders>>("NtHeaders", 
    boost::python::init<Hades::Memory::PeFile const&>())
    .def("GetBase", &NtHeadersWrap::GetBase)
    .def("IsSignatureValid", &NtHeadersWrap::IsSignatureValid)
    .def("EnsureSignatureValid", &NtHeadersWrap::EnsureSignatureValid)
    .def("GetSignature", &NtHeadersWrap::GetSignature)
    .def("SetSignature", &NtHeadersWrap::SetSignature)
    .def("GetMachine", &NtHeadersWrap::GetMachine)
    .def("SetMachine", &NtHeadersWrap::SetMachine)
    .def("GetNumberOfSections", &NtHeadersWrap::GetNumberOfSections)
    .def("SetNumberOfSections", &NtHeadersWrap::SetNumberOfSections)
    .def("GetTimeDateStamp", &NtHeadersWrap::GetTimeDateStamp)
    .def("SetTimeDateStamp", &NtHeadersWrap::SetTimeDateStamp)
    .def("GetPointerToSymbolTable", &NtHeadersWrap::GetPointerToSymbolTable)
    .def("SetPointerToSymbolTable", &NtHeadersWrap::SetPointerToSymbolTable)
    .def("GetNumberOfSymbols", &NtHeadersWrap::GetNumberOfSymbols)
    .def("SetNumberOfSymbols", &NtHeadersWrap::SetNumberOfSymbols)
    .def("GetSizeOfOptionalHeader", &NtHeadersWrap::GetSizeOfOptionalHeader)
    .def("SetSizeOfOptionalHeader", &NtHeadersWrap::SetSizeOfOptionalHeader)
    .def("GetCharacteristics", &NtHeadersWrap::GetCharacteristics)
    .def("SetCharacteristics", &NtHeadersWrap::SetCharacteristics)
    .def("GetMagic", &NtHeadersWrap::GetMagic)
    .def("SetMagic", &NtHeadersWrap::SetMagic)
    .def("GetMajorLinkerVersion", &NtHeadersWrap::GetMajorLinkerVersion)
    .def("SetMajorLinkerVersion", &NtHeadersWrap::SetMajorLinkerVersion)
    .def("GetMinorLinkerVersion", &NtHeadersWrap::GetMinorLinkerVersion)
    .def("SetMinorLinkerVersion", &NtHeadersWrap::SetMinorLinkerVersion)
    .def("GetSizeOfCode", &NtHeadersWrap::GetSizeOfCode)
    .def("SetSizeOfCode", &NtHeadersWrap::SetSizeOfCode)
    .def("GetSizeOfInitializedData", &NtHeadersWrap::GetSizeOfInitializedData)
    .def("SetSizeOfInitializedData", &NtHeadersWrap::SetSizeOfInitializedData)
    .def("GetSizeOfUninitializedData", &NtHeadersWrap::
    GetSizeOfUninitializedData)
    .def("SetSizeOfUninitializedData", &NtHeadersWrap::
    SetSizeOfUninitializedData)
    .def("GetAddressOfEntryPoint", &NtHeadersWrap::GetAddressOfEntryPoint)
    .def("SetAddressOfEntryPoint", &NtHeadersWrap::SetAddressOfEntryPoint)
    .def("GetBaseOfCode", &NtHeadersWrap::GetBaseOfCode)
    .def("SetBaseOfCode", &NtHeadersWrap::SetBaseOfCode)
#if defined(_M_IX86) 
    .def("GetBaseOfData", &NtHeadersWrap::GetBaseOfData)
    .def("SetBaseOfData", &NtHeadersWrap::SetBaseOfData)
#endif
    .def("GetImageBase", &NtHeadersWrap::GetImageBase)
    .def("SetImageBase", &NtHeadersWrap::SetImageBase)
    .def("GetSectionAlignment", &NtHeadersWrap::GetSectionAlignment)
    .def("SetSectionAlignment", &NtHeadersWrap::SetSectionAlignment)
    .def("GetFileAlignment", &NtHeadersWrap::GetFileAlignment)
    .def("SetFileAlignment", &NtHeadersWrap::SetFileAlignment)
    .def("GetMajorOperatingSystemVersion", &NtHeadersWrap::
    GetMajorOperatingSystemVersion)
    .def("SetMajorOperatingSystemVersion", &NtHeadersWrap::
    SetMajorOperatingSystemVersion)
    .def("GetMinorOperatingSystemVersion", &NtHeadersWrap::
    GetMinorOperatingSystemVersion)
    .def("SetMinorOperatingSystemVersion", &NtHeadersWrap::
    SetMinorOperatingSystemVersion)
    .def("GetMajorImageVersion", &NtHeadersWrap::GetMajorImageVersion)
    .def("SetMajorImageVersion", &NtHeadersWrap::SetMajorImageVersion)
    .def("GetMinorImageVersion", &NtHeadersWrap::GetMinorImageVersion)
    .def("SetMinorImageVersion", &NtHeadersWrap::SetMinorImageVersion)
    .def("GetMajorSubsystemVersion", &NtHeadersWrap::GetMajorSubsystemVersion)
    .def("SetMajorSubsystemVersion", &NtHeadersWrap::SetMajorSubsystemVersion)
    .def("GetMinorSubsystemVersion", &NtHeadersWrap::GetMinorSubsystemVersion)
    .def("SetMinorSubsystemVersion", &NtHeadersWrap::SetMinorSubsystemVersion)
    .def("GetWin32VersionValue", &NtHeadersWrap::GetWin32VersionValue)
    .def("SetWin32VersionValue", &NtHeadersWrap::SetWin32VersionValue)
    .def("GetSizeOfImage", &NtHeadersWrap::GetSizeOfImage)
    .def("SetSizeOfImage", &NtHeadersWrap::SetSizeOfImage)
    .def("GetSizeOfHeaders", &NtHeadersWrap::GetSizeOfHeaders)
    .def("SetSizeOfHeaders", &NtHeadersWrap::SetSizeOfHeaders)
    .def("GetCheckSum", &NtHeadersWrap::GetCheckSum)
    .def("SetCheckSum", &NtHeadersWrap::SetCheckSum)
    .def("GetSubsystem", &NtHeadersWrap::GetSubsystem)
    .def("SetSubsystem", &NtHeadersWrap::SetSubsystem)
    .def("GetDllCharacteristics", &NtHeadersWrap::GetDllCharacteristics)
    .def("SetDllCharacteristics", &NtHeadersWrap::SetDllCharacteristics)
    .def("GetSizeOfStackReserve", &NtHeadersWrap::GetSizeOfStackReserve)
    .def("SetSizeOfStackReserve", &NtHeadersWrap::SetSizeOfStackReserve)
    .def("GetSizeOfStackCommit", &NtHeadersWrap::GetSizeOfStackCommit)
    .def("SetSizeOfStackCommit", &NtHeadersWrap::SetSizeOfStackCommit)
    .def("GetSizeOfHeapReserve", &NtHeadersWrap::GetSizeOfHeapReserve)
    .def("SetSizeOfHeapReserve", &NtHeadersWrap::SetSizeOfHeapReserve)
    .def("GetSizeOfHeapCommit", &NtHeadersWrap::GetSizeOfHeapCommit)
    .def("SetSizeOfHeapCommit", &NtHeadersWrap::SetSizeOfHeapCommit)
    .def("GetLoaderFlags", &NtHeadersWrap::GetLoaderFlags)
    .def("SetLoaderFlags", &NtHeadersWrap::SetLoaderFlags)
    .def("GetNumberOfRvaAndSizes", &NtHeadersWrap::GetNumberOfRvaAndSizes)
    .def("SetNumberOfRvaAndSizes", &NtHeadersWrap::SetNumberOfRvaAndSizes)
    .def("GetDataDirectoryVirtualAddress", &NtHeadersWrap::
    GetDataDirectoryVirtualAddress)
    .def("SetDataDirectoryVirtualAddress", &NtHeadersWrap::
    SetDataDirectoryVirtualAddress)
    .def("GetDataDirectorySize", &NtHeadersWrap::GetDataDirectorySize)
    .def("SetDataDirectorySize", &NtHeadersWrap::SetDataDirectorySize)
    .def("GetHeadersRaw", &NtHeadersWrap::GetHeadersRaw)
    ;

  boost::python::enum_<Hades::Memory::NtHeaders::DataDir>("DataDir")
    .value("Export", Hades::Memory::NtHeaders::DataDir_Export)
    .value("Import", Hades::Memory::NtHeaders::DataDir_Import)
    .value("Resource", Hades::Memory::NtHeaders::DataDir_Resource)
    .value("Exception", Hades::Memory::NtHeaders::DataDir_Exception)
    .value("Security", Hades::Memory::NtHeaders::DataDir_Security)
    .value("BaseReloc", Hades::Memory::NtHeaders::DataDir_BaseReloc)
    .value("Debug", Hades::Memory::NtHeaders::DataDir_Debug)
    .value("Architecture", Hades::Memory::NtHeaders::DataDir_Architecture)
    .value("GlobalPTR", Hades::Memory::NtHeaders::DataDir_GlobalPTR)
    .value("TLS", Hades::Memory::NtHeaders::DataDir_TLS)
    .value("LoadConfig", Hades::Memory::NtHeaders::DataDir_LoadConfig)
    .value("BoundImport", Hades::Memory::NtHeaders::DataDir_BoundImport)
    .value("IAT", Hades::Memory::NtHeaders::DataDir_IAT)
    .value("DelayImport", Hades::Memory::NtHeaders::DataDir_DelayImport)
    .value("COMDescriptor", Hades::Memory::NtHeaders::DataDir_COMDescriptor)
    ;
}

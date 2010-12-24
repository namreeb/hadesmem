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
#pragma warning(push, 1)
#include <boost/python.hpp>
#include <boost/python/stl_iterator.hpp>
#pragma warning(pop)

// Hades
#include "Memory/Types.h"
#include "Memory/MemoryMgr.h"

class MemoryMgrWrap : public Hades::Memory::MemoryMgr
{
public:
  explicit MemoryMgrWrap(DWORD ProcID) 
    : Hades::Memory::MemoryMgr(ProcID)
  { }

  explicit MemoryMgrWrap(std::basic_string<TCHAR> const& ProcName) 
    : Hades::Memory::MemoryMgr(ProcName)
  { }

  explicit MemoryMgrWrap(std::basic_string<TCHAR> const& WindowName, 
    std::basic_string<TCHAR> const& ClassName) 
    : Hades::Memory::MemoryMgr(WindowName, ClassName)
  { }

  DWORD_PTR Call(DWORD_PTR Address, boost::python::object const& Args, 
    CallConv MyCallConv) const
  {
    boost::python::stl_input_iterator<DWORD_PTR> ArgsBeg(Args), ArgsEnd;

    std::vector<PVOID> ArgsNew;
    std::transform(ArgsBeg, ArgsEnd, std::back_inserter(ArgsNew), 
      [] (DWORD_PTR Current) 
    {
      return reinterpret_cast<PVOID>(Current);
    });

    return Hades::Memory::MemoryMgr::Call(reinterpret_cast<PVOID>(Address), 
      ArgsNew, MyCallConv);
  }

  DWORD_PTR Alloc(SIZE_T Size) const
  {
    return reinterpret_cast<DWORD_PTR>(Hades::Memory::MemoryMgr::Alloc(Size));
  }

  DWORD_PTR GetProcessHandle() const
  {
    return reinterpret_cast<DWORD_PTR>(Hades::Memory::MemoryMgr::
      GetProcessHandle());
  }

  DWORD_PTR GetRemoteProcAddressByName(HMODULE RemoteMod, 
    std::basic_string<TCHAR> const& Module, std::string const& Function) const
  {
    return reinterpret_cast<DWORD_PTR>(Hades::Memory::MemoryMgr::
      GetRemoteProcAddress(RemoteMod, Module, Function));
  }

  DWORD_PTR GetRemoteProcAddressByOrdinal(HMODULE RemoteMod, 
    std::basic_string<TCHAR> const& Module, WORD Function) const
  {
    return reinterpret_cast<DWORD_PTR>(Hades::Memory::MemoryMgr::
      GetRemoteProcAddress(RemoteMod, Module, Function));
  }

  template <typename T>
  T Read(DWORD_PTR Address) const
  {
    return Hades::Memory::MemoryMgr::Read<T>(reinterpret_cast<PVOID>(Address));
  }

  DWORD_PTR ReadPointer(DWORD_PTR Address) const
  {
    return Read<DWORD_PTR>(Address);
  }

  template <typename T>
  void Write(DWORD_PTR Address, T Data) const
  {
    Hades::Memory::MemoryMgr::Write(reinterpret_cast<PVOID>(Address), Data);
  }

  void WritePointer(DWORD_PTR Address, DWORD_PTR Data) const
  {
    Write(Address, Data);
  }
};

// Export MemoryMgr API
void ExportMemoryMgr()
{
  boost::python::class_<Hades::Memory::MemoryMgr>("MemoryMgrBase", 
    boost::python::no_init)
    ;

  boost::python::scope MemoryMgrScope = boost::python::class_<MemoryMgrWrap, 
    boost::python::bases<Hades::Memory::MemoryMgr>>("MemoryMgr", 
    boost::python::init<DWORD>())
    .def(boost::python::init<std::basic_string<TCHAR> const&>())
    .def(boost::python::init<std::basic_string<TCHAR> const&, 
    std::basic_string<TCHAR> const&>())
    .def("Call", &MemoryMgrWrap::Call)
    .def("ReadInt8", &MemoryMgrWrap::Read<Hades::Memory::Types::Int8>)
    .def("ReadUInt8", &MemoryMgrWrap::Read<Hades::Memory::Types::UInt8>)
    .def("ReadInt16", &MemoryMgrWrap::Read<Hades::Memory::Types::Int16>)
    .def("ReadUInt16", &MemoryMgrWrap::Read<Hades::Memory::Types::UInt16>)
    .def("ReadInt32", &MemoryMgrWrap::Read<Hades::Memory::Types::Int32>)
    .def("ReadUInt32", &MemoryMgrWrap::Read<Hades::Memory::Types::UInt32>)
    .def("ReadInt64", &MemoryMgrWrap::Read<Hades::Memory::Types::Int64>)
    .def("ReadUInt64", &MemoryMgrWrap::Read<Hades::Memory::Types::UInt64>)
    .def("ReadFloat", &MemoryMgrWrap::Read<Hades::Memory::Types::Float>)
    .def("ReadDouble", &MemoryMgrWrap::Read<Hades::Memory::Types::Double>)
    .def("ReadCharA", &MemoryMgrWrap::Read<Hades::Memory::Types::CharA>)
    .def("ReadCharW", &MemoryMgrWrap::Read<Hades::Memory::Types::CharW>)
    .def("ReadStringA", &MemoryMgrWrap::Read<Hades::Memory::Types::StringA>)
    .def("ReadStringW", &MemoryMgrWrap::Read<Hades::Memory::Types::StringW>)
    .def("ReadPointer", &MemoryMgrWrap::ReadPointer)
    .def("WriteInt8", &MemoryMgrWrap::Write<Hades::Memory::Types::Int8>)
    .def("WriteUInt8", &MemoryMgrWrap::Write<Hades::Memory::Types::UInt8>)
    .def("WriteInt16", &MemoryMgrWrap::Write<Hades::Memory::Types::Int16>)
    .def("WriteUInt16", &MemoryMgrWrap::Write<Hades::Memory::Types::UInt16>)
    .def("WriteInt32", &MemoryMgrWrap::Write<Hades::Memory::Types::Int32>)
    .def("WriteUInt32", &MemoryMgrWrap::Write<Hades::Memory::Types::UInt32>)
    .def("WriteInt64", &MemoryMgrWrap::Write<Hades::Memory::Types::Int64>)
    .def("WriteUInt64", &MemoryMgrWrap::Write<Hades::Memory::Types::UInt64>)
    .def("WriteFloat", &MemoryMgrWrap::Write<Hades::Memory::Types::Float>)
    .def("WriteDouble", &MemoryMgrWrap::Write<Hades::Memory::Types::Double>)
    .def("WriteCharA", &MemoryMgrWrap::Write<Hades::Memory::Types::CharA>)
    .def("WriteCharW", &MemoryMgrWrap::Write<Hades::Memory::Types::CharW>)
    .def("WriteStringA", &MemoryMgrWrap::Write<Hades::Memory::Types::StringA>)
    .def("WriteStringW", &MemoryMgrWrap::Write<Hades::Memory::Types::StringW>)
    .def("WritePointer", &MemoryMgrWrap::Write<Hades::Memory::Types::Pointer>)
    .def("CanRead", &MemoryMgrWrap::CanRead)
    .def("CanWrite", &MemoryMgrWrap::CanWrite)
    .def("IsGuard", &MemoryMgrWrap::IsGuard)
    .def("Alloc", &MemoryMgrWrap::Alloc)
    .def("Free", &MemoryMgrWrap::Free)
    .def("GetProcessID", &MemoryMgrWrap::GetProcessID)
    .def("GetProcessHandle", &MemoryMgrWrap::GetProcessHandle)
    .def("GetRemoteProcAddress", &MemoryMgrWrap::GetRemoteProcAddressByName)
    .def("GetRemoteProcAddress", &MemoryMgrWrap::GetRemoteProcAddressByOrdinal)
    .def("FlushCache", &MemoryMgrWrap::FlushCache)
    ;

  boost::python::enum_<Hades::Memory::MemoryMgr::CallConv>("CallConv")
    .value("CDECL", Hades::Memory::MemoryMgr::CallConv_CDECL)
    .value("STDCALL", Hades::Memory::MemoryMgr::CallConv_STDCALL)
    .value("THISCALL", Hades::Memory::MemoryMgr::CallConv_THISCALL)
    .value("FASTCALL", Hades::Memory::MemoryMgr::CallConv_FASTCALL)
    .value("X64", Hades::Memory::MemoryMgr::CallConv_X64)
    .value("Default", Hades::Memory::MemoryMgr::CallConv_Default)
    ;
}

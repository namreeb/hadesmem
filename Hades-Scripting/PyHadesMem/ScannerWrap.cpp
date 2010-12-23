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
#pragma warning(pop)

// Hades
#include "Hades-Memory/Types.h"
#include "Hades-Memory/Scanner.h"

class ScannerWrap : public Hades::Memory::Scanner
{
public:
  explicit ScannerWrap(Hades::Memory::MemoryMgr const& MyMemory, 
    DWORD_PTR Start, DWORD_PTR End) 
    : Hades::Memory::Scanner(MyMemory, reinterpret_cast<PVOID>(Start), 
    reinterpret_cast<PVOID>(End))
  { }

  template <typename T>
  DWORD_PTR Find(T Data) const
  {
    return reinterpret_cast<DWORD_PTR>(Hades::Memory::Scanner::Find(Data));
  }

  DWORD_PTR FindPointer(DWORD_PTR Data) const
  {
    return Find(Data);
  }

  template <typename T>
  std::vector<DWORD_PTR> FindAll(T Data) const
  {
    std::vector<PVOID> Temp(Hades::Memory::Scanner::FindAll(Data));
    std::vector<DWORD_PTR> New(reinterpret_cast<DWORD_PTR*>(&Temp[0]), 
      reinterpret_cast<DWORD_PTR*>(&Temp[0]) + Temp.size());
    return New;
  }

  std::vector<DWORD_PTR> FindAllPointer(DWORD_PTR Data) const
  {
    return FindAll(Data);
  }
};

// Export Scanner API
void ExportScanner()
{
  boost::python::class_<Hades::Memory::Scanner>("ScannerBase", 
    boost::python::no_init)
    ;

  boost::python::class_<ScannerWrap>("Scanner", boost::python::init<
    Hades::Memory::MemoryMgr const&, DWORD_PTR, DWORD_PTR>())
    .def("FindInt8", &ScannerWrap::Find<Hades::Memory::Types::Int8>)
    .def("FindUInt8", &ScannerWrap::Find<Hades::Memory::Types::UInt8>)
    .def("FindInt16", &ScannerWrap::Find<Hades::Memory::Types::Int16>)
    .def("FindUInt16", &ScannerWrap::Find<Hades::Memory::Types::UInt16>)
    .def("FindInt32", &ScannerWrap::Find<Hades::Memory::Types::Int32>)
    .def("FindUInt32", &ScannerWrap::Find<Hades::Memory::Types::UInt32>)
    .def("FindInt64", &ScannerWrap::Find<Hades::Memory::Types::Int64>)
    .def("FindUInt64", &ScannerWrap::Find<Hades::Memory::Types::UInt64>)
    .def("FindFloat", &ScannerWrap::Find<Hades::Memory::Types::Float>)
    .def("FindDouble", &ScannerWrap::Find<Hades::Memory::Types::Double>)
    .def("FindCharA", &ScannerWrap::Find<Hades::Memory::Types::CharA>)
    .def("FindCharW", &ScannerWrap::Find<Hades::Memory::Types::CharW>)
    .def("FindStringA", &ScannerWrap::Find<Hades::Memory::Types::StringA>)
    .def("FindStringW", &ScannerWrap::Find<Hades::Memory::Types::StringW>)
    .def("FindPointer", &ScannerWrap::FindPointer)
    .def("FindAllInt8", &ScannerWrap::FindAll<Hades::Memory::Types::Int8>)
    .def("FindAllUInt8", &ScannerWrap::FindAll<Hades::Memory::Types::UInt8>)
    .def("FindAllInt16", &ScannerWrap::FindAll<Hades::Memory::Types::Int16>)
    .def("FindAllUInt16", &ScannerWrap::FindAll<Hades::Memory::Types::UInt16>)
    .def("FindAllInt32", &ScannerWrap::FindAll<Hades::Memory::Types::Int32>)
    .def("FindAllUInt32", &ScannerWrap::FindAll<Hades::Memory::Types::UInt32>)
    .def("FindAllInt64", &ScannerWrap::FindAll<Hades::Memory::Types::Int64>)
    .def("FindAllUInt64", &ScannerWrap::FindAll<Hades::Memory::Types::UInt64>)
    .def("FindAllFloat", &ScannerWrap::FindAll<Hades::Memory::Types::Float>)
    .def("FindAllDouble", &ScannerWrap::FindAll<Hades::Memory::Types::Double>)
    .def("FindAllCharA", &ScannerWrap::FindAll<Hades::Memory::Types::CharA>)
    .def("FindAllCharW", &ScannerWrap::FindAll<Hades::Memory::Types::CharW>)
    .def("FindAllStringA", &ScannerWrap::FindAll<Hades::Memory::Types::StringA>)
    .def("FindAllStringW", &ScannerWrap::FindAll<Hades::Memory::Types::StringW>)
    .def("FindAllPointer", &ScannerWrap::FindAllPointer)
    ;
}

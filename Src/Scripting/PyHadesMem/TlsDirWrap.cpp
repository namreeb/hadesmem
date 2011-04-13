/*
This file is part of HadesMem.
Copyright (C) 2011 Joshua Boyce (a.k.a. RaptorFactor).
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
#include "HadesMemory/TlsDir.hpp"

class TlsDirWrap : public Hades::Memory::TlsDir
{
public:
  explicit TlsDirWrap(Hades::Memory::PeFile const& MyPeFile) 
    : Hades::Memory::TlsDir(MyPeFile)
  { }

  std::vector<DWORD_PTR> GetCallbacks() const
  {
    std::vector<PIMAGE_TLS_CALLBACK> Temp(Hades::Memory::TlsDir::
      GetCallbacks());
    std::vector<DWORD_PTR> New(reinterpret_cast<DWORD_PTR*>(Temp.data()), 
      reinterpret_cast<DWORD_PTR*>(Temp.data()) + Temp.size());
    return New;
  }

  DWORD_PTR GetBase() const
  {
    return reinterpret_cast<DWORD_PTR>(Hades::Memory::TlsDir::GetBase());
  }
};

// Export TlsDir API
void ExportTlsDir()
{
  boost::python::class_<Hades::Memory::TlsDir>("TlsDirBase", 
    boost::python::no_init)
    ;

  boost::python::class_<TlsDirWrap, boost::python::bases<
    Hades::Memory::TlsDir>>("TlsDir", boost::python::init<
    Hades::Memory::PeFile const&>())
    .def("IsValid", &TlsDirWrap::IsValid)
    .def("EnsureValid", &TlsDirWrap::EnsureValid)
    .def("GetStartAddressOfRawData", &TlsDirWrap::GetStartAddressOfRawData)
    .def("GetEndAddressOfRawData", &TlsDirWrap::GetEndAddressOfRawData)
    .def("GetAddressOfIndex", &TlsDirWrap::GetAddressOfIndex)
    .def("GetAddressOfCallBacks", &TlsDirWrap::GetAddressOfCallBacks)
    .def("GetSizeOfZeroFill", &TlsDirWrap::GetSizeOfZeroFill)
    .def("GetCharacteristics", &TlsDirWrap::GetCharacteristics)
    .def("GetCallbacks", &TlsDirWrap::GetCallbacks)
    .def("GetBase", &TlsDirWrap::GetBase)
    .def("GetTlsDirRaw", &TlsDirWrap::GetTlsDirRaw)
    ;
}

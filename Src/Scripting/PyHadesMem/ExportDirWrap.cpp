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

// C++ Standard Library
#include <cmath> // GCC workaround

// Boost
#ifdef _MSC_VER
#pragma warning(push, 1)
#endif // #ifdef _MSC_VER
#include <boost/python.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif // #ifdef _MSC_VER

// Hades
#include "Memory/ExportDir.hpp"
#include "Memory/ExportEnum.hpp"

class ExportDirWrap : public Hades::Memory::ExportDir
{
public:
  explicit ExportDirWrap(Hades::Memory::PeFile const& MyPeFile)
    : Hades::Memory::ExportDir(MyPeFile)
  { }

  DWORD_PTR GetBase() const
  {
    return reinterpret_cast<DWORD_PTR>(Hades::Memory::ExportDir::GetBase());
  }
};

class ExportWrap : public Hades::Memory::Export
{
public:
  explicit ExportWrap(Hades::Memory::PeFile const& MyPeFile, DWORD Number)
    : Hades::Memory::Export(MyPeFile, Number)
  { }

  DWORD_PTR GetVa() const
  {
    return reinterpret_cast<DWORD_PTR>(Hades::Memory::Export::GetVa());
  }
};

struct ExportDirIterWrap
{
  static ExportWrap next(Hades::Memory::ExportIter& o)
  {
    if (!*o)
    {
      PyErr_SetString(PyExc_StopIteration, "No more data.");
      boost::python::throw_error_already_set();
    }

    auto MyExport(*static_cast<ExportWrap*>(&**o));

    ++o;

    return MyExport;
  }

  static boost::python::object pass_through(boost::python::object const& o) 
  {
    return o;
  }

  static void wrap(const char* python_name)
  {
    boost::python::class_<Hades::Memory::ExportIter>(python_name, 
      boost::python::init<Hades::Memory::PeFile const&>())
      .def("next", next)
      .def("__iter__", pass_through)
      ;
  }
};

// Export ExportDir API
void ExportExportDir()
{
  boost::python::class_<Hades::Memory::ExportDir>("ExportDirBase", 
    boost::python::no_init)
    ;

  boost::python::class_<ExportDirWrap, boost::python::bases<
    Hades::Memory::ExportDir>>("ExportDir", boost::python::init<
    Hades::Memory::PeFile const&>())
    .def("IsValid", &ExportDirWrap::IsValid)
    .def("EnsureValid", &ExportDirWrap::EnsureValid)
    .def("GetCharacteristics", &ExportDirWrap::GetCharacteristics)
    .def("SetCharacteristics", &ExportDirWrap::SetCharacteristics)
    .def("GetTimeDateStamp", &ExportDirWrap::GetTimeDateStamp)
    .def("SetTimeDateStamp", &ExportDirWrap::SetTimeDateStamp)
    .def("GetMajorVersion", &ExportDirWrap::GetMajorVersion)
    .def("SetMajorVersion", &ExportDirWrap::SetMajorVersion)
    .def("GetMinorVersion", &ExportDirWrap::GetMinorVersion)
    .def("SetMinorVersion", &ExportDirWrap::SetMinorVersion)
    .def("GetName", &ExportDirWrap::GetName)
    .def("GetOrdinalBase", &ExportDirWrap::GetOrdinalBase)
    .def("SetOrdinalBase", &ExportDirWrap::SetOrdinalBase)
    .def("GetNumberOfFunctions", &ExportDirWrap::GetNumberOfFunctions)
    .def("SetNumberOfFunctions", &ExportDirWrap::SetNumberOfFunctions)
    .def("GetNumberOfNames", &ExportDirWrap::GetNumberOfNames)
    .def("SetNumberOfNames", &ExportDirWrap::SetNumberOfNames)
    .def("GetAddressOfFunctions", &ExportDirWrap::GetAddressOfFunctions)
    .def("SetAddressOfFunctions", &ExportDirWrap::SetAddressOfFunctions)
    .def("GetAddressOfNames", &ExportDirWrap::GetAddressOfNames)
    .def("SetAddressOfNames", &ExportDirWrap::SetAddressOfNames)
    .def("GetAddressOfNameOrdinals", &ExportDirWrap::GetAddressOfNameOrdinals)
    .def("SetAddressOfNameOrdinals", &ExportDirWrap::SetAddressOfNameOrdinals)
    .def("GetBase", &ExportDirWrap::GetBase)
    .def("GetExportDirRaw", &ExportDirWrap::GetExportDirRaw)
    ;

  boost::python::class_<Hades::Memory::Export>("ExportBase", 
    boost::python::no_init)
    ;

  boost::python::class_<ExportWrap, boost::python::bases<
    Hades::Memory::Export>>("Export", boost::python::init<
    Hades::Memory::PeFile const&, DWORD>())
    .def("GetRva", &ExportWrap::GetRva)
    .def("GetVa", &ExportWrap::GetVa)
    .def("GetName", &ExportWrap::GetName)
    .def("GetForwarder", &ExportWrap::GetForwarder)
    .def("GetForwarderModule", &ExportWrap::GetForwarderModule)
    .def("GetForwarderFunction", &ExportWrap::GetForwarderFunction)
    .def("GetOrdinal", &ExportWrap::GetOrdinal)
    .def("ByName", &ExportWrap::ByName)
    .def("Forwarded", &ExportWrap::Forwarded)
    ;

  ExportDirIterWrap::wrap("ExportIter"); 
}

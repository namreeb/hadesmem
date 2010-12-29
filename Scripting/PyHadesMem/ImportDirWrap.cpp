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
#include "Memory/ImportDir.hpp"
#include "Memory/ImportEnum.hpp"

class ImportDirWrap : public Hades::Memory::ImportDir
{
public:
  ImportDirWrap(Hades::Memory::PeFile const& MyPeFile, DWORD_PTR ImpDesc)
    : Hades::Memory::ImportDir(MyPeFile, 
    reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(ImpDesc))
  { }

  DWORD_PTR GetBase() const
  {
    return reinterpret_cast<DWORD_PTR>(Hades::Memory::ImportDir::GetBase());
  }
};

struct ImportDirIterWrap
{
  static ImportDirWrap next(Hades::Memory::ImportDirIter& o)
  {
    if (!*o)
    {
      PyErr_SetString(PyExc_StopIteration, "No more data.");
      boost::python::throw_error_already_set();
    }

    auto MyImportDir(*static_cast<ImportDirWrap*>(&**o));

    ++o;

    return MyImportDir;
  }

  static boost::python::object pass_through(boost::python::object const& o) 
  {
    return o;
  }

  static void wrap(const char* python_name)
  {
    boost::python::class_<Hades::Memory::ImportDirIter>(python_name, 
      boost::python::init<Hades::Memory::PeFile const&>())
      .def("next", next)
      .def("__iter__", pass_through)
      ;
  }
};

class ImportThunkWrap : public Hades::Memory::ImportThunk
{
public:
  ImportThunkWrap(Hades::Memory::PeFile const& MyPeFile, DWORD_PTR Thunk)
    : Hades::Memory::ImportThunk(MyPeFile, reinterpret_cast<PVOID>(Thunk))
  { }
};

struct ImportThunkIterWrap
{
  static ImportThunkWrap next(Hades::Memory::ImportThunkIter& o)
  {
    if (!*o)
    {
      PyErr_SetString(PyExc_StopIteration, "No more data.");
      boost::python::throw_error_already_set();
    }

    auto MyImport(*static_cast<ImportThunkWrap*>(&**o));

    ++o;

    return MyImport;
  }

  static boost::python::object pass_through(boost::python::object const& o) 
  {
    return o;
  }

  static void wrap(const char* python_name)
  {
    boost::python::class_<Hades::Memory::ImportThunkIter>(python_name, 
      boost::python::init<Hades::Memory::PeFile const&, DWORD>())
      .def("next", next)
      .def("__iter__", pass_through)
      ;
  }
};

// Export ImportDir API
void ExportImportDir()
{
  boost::python::class_<Hades::Memory::ImportDir>("ImportDirBase", 
    boost::python::no_init)
    ;

  boost::python::class_<ImportDirWrap, boost::python::bases<
    Hades::Memory::ImportDir>>("ImportDir", 
    boost::python::init<Hades::Memory::PeFile const&, DWORD_PTR>())
    .def("IsValid", &ImportDirWrap::IsValid)
    .def("EnsureValid", &ImportDirWrap::EnsureValid)
    .def("GetBase", &ImportDirWrap::GetBase)
    .def("Advance", &ImportDirWrap::Advance)
    .def("GetCharacteristics", &ImportDirWrap::GetCharacteristics)
    .def("GetTimeDateStamp", &ImportDirWrap::GetTimeDateStamp)
    .def("GetForwarderChain", &ImportDirWrap::GetForwarderChain)
    .def("GetNameRaw", &ImportDirWrap::GetNameRaw)
    .def("GetName", &ImportDirWrap::GetName)
    .def("GetFirstThunk", &ImportDirWrap::GetFirstThunk)
    ;

  ImportDirIterWrap::wrap("ImportDirIter"); 

  boost::python::class_<Hades::Memory::ImportThunk>("ImportBase", 
    boost::python::no_init)
    ;

  boost::python::class_<ImportThunkWrap, boost::python::bases<
    Hades::Memory::ImportThunk>>("Import", boost::python::init<
    Hades::Memory::PeFile const&, DWORD_PTR>())
    .def("IsValid", &ImportThunkWrap::IsValid)
    .def("Advance", &ImportThunkWrap::Advance)
    .def("GetAddressOfData", &ImportThunkWrap::GetAddressOfData)
    .def("GetOrdinalRaw", &ImportThunkWrap::GetOrdinalRaw)
    .def("ByOrdinal", &ImportThunkWrap::ByOrdinal)
    .def("GetOrdinal", &ImportThunkWrap::GetOrdinal)
    .def("GetFunction", &ImportThunkWrap::GetFunction)
    .def("GetHint", &ImportThunkWrap::GetHint)
    .def("GetName", &ImportThunkWrap::GetName)
    .def("SetFunction", &ImportThunkWrap::SetFunction)
    ;

  ImportThunkIterWrap::wrap("ImportIter"); 
}

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
#include "HadesMemory/Section.hpp"
#include "HadesMemory/SectionEnum.hpp"

class SectionWrap : public Hades::Memory::Section
{
public:
  SectionWrap(Hades::Memory::PeFile const& MyPeFile, WORD Number)
    : Hades::Memory::Section(MyPeFile, Number)
  { }

  DWORD_PTR GetBase() const
  {
    return reinterpret_cast<DWORD_PTR>(Hades::Memory::Section::GetBase());
  }
};

struct SectionIterWrap
{
  static SectionWrap next(Hades::Memory::SectionIter& o)
  {
    if (!*o)
    {
      PyErr_SetString(PyExc_StopIteration, "No more data.");
      boost::python::throw_error_already_set();
    }

    auto MySection(*static_cast<SectionWrap*>(&**o));

    ++o;

    return MySection;
  }

  static boost::python::object pass_through(boost::python::object const& o) 
  {
    return o;
  }

  static void wrap(const char* python_name)
  {
    boost::python::class_<Hades::Memory::SectionIter>(python_name, 
      boost::python::init<Hades::Memory::PeFile const&>())
      .def("next", next)
      .def("__iter__", pass_through)
      ;
  }
};

// Export Section API
void ExportSection()
{
  boost::python::class_<Hades::Memory::Section>("SectionBase", 
    boost::python::no_init)
    ;

  boost::python::class_<SectionWrap, boost::python::bases<
    Hades::Memory::Section>>("Section", boost::python::init<
    Hades::Memory::PeFile const&, WORD>())
    .def("GetName", &Hades::Memory::Section::GetName)
    .def("SetName", &Hades::Memory::Section::SetName)
    .def("GetVirtualAddress", &SectionWrap::GetVirtualAddress)
    .def("SetVirtualAddress", &SectionWrap::SetVirtualAddress)
    .def("GetVirtualSize", &SectionWrap::GetVirtualSize)
    .def("SetVirtualSize", &SectionWrap::SetVirtualSize)
    .def("GetSizeOfRawData", &SectionWrap::GetSizeOfRawData)
    .def("SetSizeOfRawData", &SectionWrap::SetSizeOfRawData)
    .def("GetPointerToRawData", &SectionWrap::GetPointerToRawData)
    .def("SetPointerToRawData", &SectionWrap::SetPointerToRawData)
    .def("GetPointerToRelocations", &SectionWrap::GetPointerToRelocations)
    .def("SetPointerToRelocations", &SectionWrap::SetPointerToRelocations)
    .def("GetPointerToLinenumbers", &SectionWrap::GetPointerToLinenumbers)
    .def("SetPointerToLinenumbers", &SectionWrap::SetPointerToLinenumbers)
    .def("GetNumberOfRelocations", &SectionWrap::GetNumberOfRelocations)
    .def("SetNumberOfRelocations", &SectionWrap::SetNumberOfRelocations)
    .def("GetNumberOfLinenumbers", &SectionWrap::GetNumberOfLinenumbers)
    .def("SetNumberOfLinenumbers", &SectionWrap::SetNumberOfLinenumbers)
    .def("GetCharacteristics", &SectionWrap::GetCharacteristics)
    .def("SetCharacteristics", &SectionWrap::SetCharacteristics)
    .def("GetBase", &SectionWrap::GetBase)
    .def("GetSectionHeaderRaw", &SectionWrap::GetSectionHeaderRaw)
    ;

  SectionIterWrap::wrap("SectionIter"); 
}

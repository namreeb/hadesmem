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
#include "HadesMemory/Region.hpp"

class RegionWrap : public Hades::Memory::Region
{
public:
  RegionWrap(Hades::Memory::MemoryMgr const& MyMem, DWORD_PTR Base) 
    : Hades::Memory::Region(MyMem, reinterpret_cast<PVOID>(Base))
  { }

  DWORD_PTR GetBase() const
  {
    return reinterpret_cast<DWORD_PTR>(Hades::Memory::Region::GetBase());
  }

  DWORD_PTR GetAllocBase() const
  {
    return reinterpret_cast<DWORD_PTR>(Hades::Memory::Region::GetBase());
  }
  
  void Dump(std::wstring const& Path) const
  {
    return Hades::Memory::Region::Dump(Path);
  }
};

struct RegionIterWrap
{
  static RegionWrap next(Hades::Memory::RegionListIter& o)
  {
    if (!*o)
    {
      PyErr_SetString(PyExc_StopIteration, "No more data.");
      boost::python::throw_error_already_set();
    }

    auto MyRegionWrap(*static_cast<RegionWrap*>(&**o));

    ++o;

    return MyRegionWrap;
  }

  static boost::python::object pass_through(boost::python::object const& o) 
  {
    return o;
  }

  static void wrap(const char* python_name)
  {
    boost::python::class_<Hades::Memory::RegionListIter>(python_name, 
      boost::python::init<Hades::Memory::MemoryMgr const&>())
      .def("next", next)
      .def("__iter__", pass_through)
      ;
  }
};

// Export Region API
void ExportRegion()
{
  boost::python::class_<Hades::Memory::Region>("RegionBase", 
    boost::python::no_init)
    ;

  boost::python::class_<RegionWrap, boost::python::bases<Hades::Memory::
    Region>>("Region", boost::python::init<Hades::Memory::MemoryMgr const&, 
    DWORD_PTR>())
    .def("GetBase", &RegionWrap::GetBase)
    .def("GetAllocBase", &RegionWrap::GetAllocBase)
    .def("GetAllocProtect", &RegionWrap::GetAllocProtect)
    .def("GetSize", &RegionWrap::GetSize)
    .def("GetState", &RegionWrap::GetState)
    .def("GetProtect", &RegionWrap::GetProtect)
    .def("GetType", &RegionWrap::GetType)
    .def("Dump", &RegionWrap::Dump)
    ;

  RegionIterWrap::wrap("RegionIter"); 
}

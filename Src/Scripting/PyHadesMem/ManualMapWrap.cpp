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
#include "HadesMemory/ManualMap.hpp"

class ManualMapWrap : public Hades::Memory::ManualMap
{
public:
  explicit ManualMapWrap(Hades::Memory::MemoryMgr const& MyMem) 
    : Hades::Memory::ManualMap(MyMem)
  { }

  DWORD_PTR Map(std::wstring const& Path, 
    std::string const& Export, bool InjectHelper) const
  {
    return reinterpret_cast<DWORD_PTR>(Hades::Memory::ManualMap::Map(Path, 
      Export, InjectHelper));
  }
};

// Export ManualMap API
void ExportManualMap()
{
  boost::python::class_<Hades::Memory::ManualMap>("ManualMapBase", 
    boost::python::no_init)
    ;

  boost::python::class_<ManualMapWrap, boost::python::bases<Hades::Memory::
    ManualMap>>("ManualMap", boost::python::init<
    Hades::Memory::MemoryMgr const&>())
    .def("Map", &ManualMapWrap::Map)
    ;
}

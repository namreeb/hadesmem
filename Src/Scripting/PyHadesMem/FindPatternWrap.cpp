/*
This file is part of HadesMem.
Copyright (C) 2010 Joshua Boyce (aka RaptorFactor, Cypherjb, Cypher, Chazwazza).
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
#include "HadesMemory/FindPattern.hpp"

class FindPatternWrap : public Hades::Memory::FindPattern
{
public:
  explicit FindPatternWrap(Hades::Memory::MemoryMgr const& MyMem) 
    : Hades::Memory::FindPattern(MyMem)
  { }

  FindPatternWrap(Hades::Memory::MemoryMgr const& MyMem, DWORD_PTR ModuleBase) 
    : Hades::Memory::FindPattern(MyMem, reinterpret_cast<HMODULE>(ModuleBase))
  { }

  DWORD_PTR Find(std::wstring const& Data, 
    std::wstring const& Mask) const
  {
    return reinterpret_cast<DWORD_PTR>(Hades::Memory::FindPattern::Find(Data, 
      Mask));
  }

  void LoadFromXML(std::wstring const& Path)
  {
    return Hades::Memory::FindPattern::LoadFromXML(Path);
  }

  DWORD_PTR GetAddress(std::wstring const& Name) const
  {
    Hades::Memory::FindPattern const& Me = *this;
    return reinterpret_cast<DWORD_PTR>(Me[Name]);
  }
};

// Export FindPattern API
void ExportFindPattern()
{
  boost::python::class_<Hades::Memory::FindPattern>("FindPatternBase", 
    boost::python::no_init)
    ;

  boost::python::class_<FindPatternWrap, boost::python::bases<Hades::Memory::
    FindPattern>>("FindPattern", boost::python::init<
    Hades::Memory::MemoryMgr const&>())
    .def(boost::python::init<Hades::Memory::MemoryMgr const&, DWORD_PTR>())
    .def("Find", &FindPatternWrap::Find)
    .def("LoadFromXML", &FindPatternWrap::LoadFromXML)
    .def("GetAddresses", &FindPatternWrap::GetAddresses)
    .def("GetAddress", &FindPatternWrap::GetAddress)
    ;
}

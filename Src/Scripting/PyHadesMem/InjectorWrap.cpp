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
#include "HadesMemory/Injector.hpp"

template <class T1, class T2, class T3>
boost::python::tuple tuple_to_python(std::tuple<T1, T2, T3> const& x)
{
  return boost::python::make_tuple(std::get<0>(x), std::get<1>(x), 
    std::get<2>(x));
}

template <class T>
struct tupleconverter
{
  static PyObject* convert(T const& x)
  {
    return boost::python::incref(tuple_to_python(x).ptr());
  }
};

std::tuple<Hades::Memory::MemoryMgr, DWORD_PTR, DWORD_PTR> CreateAndInject(
  std::wstring const& Path, 
  std::wstring const& WorkDir, 
  std::wstring const& Args, 
  std::wstring const& Module, 
  std::string const& Export)
{
  auto InjectData(Hades::Memory::CreateAndInject(Path, WorkDir, Args, Module, 
    Export));
  Hades::Memory::MemoryMgr MyMemory(std::get<0>(InjectData));
  DWORD_PTR ModuleBase = reinterpret_cast<DWORD_PTR>(std::get<1>(InjectData));
  DWORD_PTR ExportRet = std::get<2>(InjectData);
  return std::make_tuple(MyMemory, ModuleBase, ExportRet);
}

class InjectorWrap : public Hades::Memory::Injector
{
public:
  explicit InjectorWrap(Hades::Memory::MemoryMgr const& MyMem) 
    : Hades::Memory::Injector(MyMem)
  { }

  DWORD_PTR InjectDll(std::wstring const& Path, 
    bool PathResolution) const
  {
    return reinterpret_cast<DWORD_PTR>(Hades::Memory::Injector::InjectDll(Path, 
      PathResolution));
  }

  DWORD_PTR CallExport(std::wstring const& Path, 
    DWORD_PTR ModuleRemote, std::string const& Export) const
  {
    return Hades::Memory::Injector::CallExport(Path, reinterpret_cast<HMODULE>(
      ModuleRemote), Export);
  }
};

// Export Injector API
void ExportInjector()
{
  boost::python::class_<Hades::Memory::Injector>("InjectorBase", 
    boost::python::no_init)
    ;

  boost::python::class_<InjectorWrap, boost::python::bases<Hades::Memory::
    Injector>>("Injector", boost::python::init<
    Hades::Memory::MemoryMgr const&>())
    .def("InjectDll", &InjectorWrap::InjectDll)
    .def("CallExport", &InjectorWrap::CallExport)
    ;

  boost::python::def("CreateAndInject", &CreateAndInject);

  boost::python::to_python_converter<std::tuple<Hades::Memory::MemoryMgr, 
    DWORD_PTR, DWORD_PTR>, tupleconverter<std::tuple<Hades::Memory::MemoryMgr, 
    DWORD_PTR, DWORD_PTR>>>();
}

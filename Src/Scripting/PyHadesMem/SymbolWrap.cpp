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
#include "HadesMemory/Symbol.hpp"

class SymbolsWrap : public Hades::Memory::Symbols
{
public:
  explicit SymbolsWrap(Hades::Memory::MemoryMgr const& MyMem) 
    : Hades::Memory::Symbols(MyMem)
  { }

  SymbolsWrap(Hades::Memory::MemoryMgr const& MyMem, 
    std::wstring const& SearchPath) 
    : Hades::Memory::Symbols(MyMem, SearchPath)
  { }
  
  DWORD_PTR GetAddress(std::wstring const& Name)
  {
    return reinterpret_cast<DWORD_PTR>(Hades::Memory::Symbols::GetAddress(Name));
  }
};

// Export Symbol API
void ExportSymbol()
{
  boost::python::class_<Hades::Memory::Symbols, boost::noncopyable>(
    "SymbolsBase", boost::python::no_init)
    ;

  boost::python::class_<SymbolsWrap, boost::python::bases<Hades::Memory::
    Symbols>, boost::noncopyable>("Symbols", boost::python::init<
    Hades::Memory::MemoryMgr const&>())
    .def(boost::python::init<Hades::Memory::MemoryMgr const&, 
      std::wstring const&>())
    .def("LoadForModule", &SymbolsWrap::LoadForModule)
    .def("GetAddress", &SymbolsWrap::GetAddress)
    ;
}

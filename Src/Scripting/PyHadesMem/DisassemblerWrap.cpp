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
#include "HadesMemory/Disassembler.hpp"

class DisassemblerWrap : public Hades::Memory::Disassembler
{
public:
  explicit DisassemblerWrap(Hades::Memory::MemoryMgr const& MyMem) 
    : Hades::Memory::Disassembler(MyMem)
  { }

  std::vector<std::basic_string<TCHAR>> DisassembleToStr(DWORD_PTR Address, 
    DWORD_PTR NumInstructions) const
  {
    return Hades::Memory::Disassembler::DisassembleToStr(
      reinterpret_cast<PVOID>(Address), NumInstructions);
  }

  std::vector<Hades::Memory::DisasmData> Disassemble(DWORD_PTR Address, 
    DWORD_PTR NumInstructions) const
  {
    return Hades::Memory::Disassembler::Disassemble(reinterpret_cast<PVOID>(
      Address), NumInstructions);
  }
};

// Export Disassembler API
void ExportDisassembler()
{
  boost::python::class_<Hades::Memory::Disassembler>("DisassemblerBase", 
    boost::python::no_init)
    ;

  boost::python::class_<DisassemblerWrap, boost::python::bases<Hades::Memory::
    Disassembler>>("Disassembler", boost::python::init<
    Hades::Memory::MemoryMgr const&>())
    .def("DisassembleToStr", &DisassemblerWrap::DisassembleToStr)
    .def("Disassemble", &DisassemblerWrap::Disassemble)
    ;
}

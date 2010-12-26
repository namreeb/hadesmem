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
#include "Memory/PeFile.h"

class PeFileWrap : public Hades::Memory::PeFile
{
public:
  PeFileWrap(Hades::Memory::MemoryMgr const& MyMemory, DWORD_PTR Address, 
    Hades::Memory::PeFile::FileType MyFileType)
    : Hades::Memory::PeFile(MyMemory, reinterpret_cast<PVOID>(Address), 
    MyFileType)
  { }

  DWORD_PTR GetBase() const
  {
    return reinterpret_cast<DWORD_PTR>(Hades::Memory::PeFile::GetBase());
  }

  DWORD_PTR RvaToVa(DWORD Rva) const
  {
    return reinterpret_cast<DWORD_PTR>(Hades::Memory::PeFile::RvaToVa(Rva));
  }
};

// Export PeFile API
void ExportPeFile()
{
  boost::python::class_<Hades::Memory::PeFile>("PeFileBase", 
    boost::python::no_init)
    ;

  boost::python::scope PeFileScope = boost::python::class_<PeFileWrap, 
    boost::python::bases<Hades::Memory::PeFile>>("PeFile", 
    boost::python::init<Hades::Memory::MemoryMgr const&, DWORD_PTR, 
    Hades::Memory::PeFile::FileType>())
    .def("GetMemoryMgr", &PeFileWrap::GetMemoryMgr)
    .def("GetBase", &PeFileWrap::GetBase)
    .def("RvaToVa", &PeFileWrap::RvaToVa)
    .def("GetType", &PeFileWrap::GetType)
    ;

  boost::python::enum_<Hades::Memory::PeFile::FileType>("FileType")
    .value("Image", Hades::Memory::PeFile::FileType_Image)
    .value("Data", Hades::Memory::PeFile::FileType_Data)
    ;
}

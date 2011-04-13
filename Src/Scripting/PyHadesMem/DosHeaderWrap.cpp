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
#include "HadesMemory/DosHeader.hpp"

// Export DosHeader API
void ExportDosHeader()
{
  boost::python::class_<Hades::Memory::DosHeader>("DosHeader", 
    boost::python::init<Hades::Memory::PeFile const&>())
    .def("IsMagicValid", &Hades::Memory::DosHeader::IsMagicValid)
    .def("EnsureMagicValid", &Hades::Memory::DosHeader::EnsureMagicValid)
    .def("GetMagic", &Hades::Memory::DosHeader::GetMagic)
    .def("GetBytesOnLastPage", &Hades::Memory::DosHeader::GetBytesOnLastPage)
    .def("GetPagesInFile", &Hades::Memory::DosHeader::GetPagesInFile)
    .def("GetRelocations", &Hades::Memory::DosHeader::GetRelocations)
    .def("GetSizeOfHeaderInParagraphs", &Hades::Memory::DosHeader::
      GetSizeOfHeaderInParagraphs)
    .def("GetMinExtraParagraphs", &Hades::Memory::DosHeader::
      GetMinExtraParagraphs)
    .def("GetMaxExtraParagraphs", &Hades::Memory::DosHeader::
      GetMaxExtraParagraphs)
    .def("GetInitialSS", &Hades::Memory::DosHeader::GetInitialSS)
    .def("GetInitialSP", &Hades::Memory::DosHeader::GetInitialSP)
    .def("GetChecksum", &Hades::Memory::DosHeader::GetChecksum)
    .def("GetInitialIP", &Hades::Memory::DosHeader::GetInitialIP)
    .def("GetInitialCS", &Hades::Memory::DosHeader::GetInitialCS)
    .def("GetRelocTableFileAddr", &Hades::Memory::DosHeader::
      GetRelocTableFileAddr)
    .def("GetOverlayNum", &Hades::Memory::DosHeader::GetOverlayNum)
    .def("GetReservedWords1", &Hades::Memory::DosHeader::GetReservedWords1)
    .def("GetOEMID", &Hades::Memory::DosHeader::GetOEMID)
    .def("GetOEMInfo", &Hades::Memory::DosHeader::GetOEMInfo)
    .def("GetReservedWords2", &Hades::Memory::DosHeader::GetReservedWords2)
    .def("GetNewHeaderOffset", &Hades::Memory::DosHeader::GetNewHeaderOffset)
    .def("SetMagic", &Hades::Memory::DosHeader::SetMagic)
    .def("SetBytesOnLastPage", &Hades::Memory::DosHeader::SetBytesOnLastPage)
    .def("SetPagesInFile", &Hades::Memory::DosHeader::SetPagesInFile)
    .def("SetRelocations", &Hades::Memory::DosHeader::SetRelocations)
    .def("SetSizeOfHeaderInParagraphs", &Hades::Memory::DosHeader::
      SetSizeOfHeaderInParagraphs)
    .def("SetMinExtraParagraphs", &Hades::Memory::DosHeader::
    SetMinExtraParagraphs)
    .def("SetMaxExtraParagraphs", &Hades::Memory::DosHeader::
      SetMaxExtraParagraphs)
    .def("SetInitialSS", &Hades::Memory::DosHeader::SetInitialSS)
    .def("SetInitialSP", &Hades::Memory::DosHeader::SetInitialSP)
    .def("SetChecksum", &Hades::Memory::DosHeader::SetChecksum)
    .def("SetInitialIP", &Hades::Memory::DosHeader::SetInitialIP)
    .def("SetInitialCS", &Hades::Memory::DosHeader::SetInitialCS)
    .def("SetRelocTableFileAddr", &Hades::Memory::DosHeader::
      SetRelocTableFileAddr)
    .def("SetOverlayNum", &Hades::Memory::DosHeader::SetOverlayNum)
    .def("SetReservedWords1", &Hades::Memory::DosHeader::SetReservedWords1)
    .def("SetOEMID", &Hades::Memory::DosHeader::SetOEMID)
    .def("SetOEMInfo", &Hades::Memory::DosHeader::SetOEMInfo)
    .def("SetReservedWords2", &Hades::Memory::DosHeader::SetReservedWords2)
    .def("SetNewHeaderOffset", &Hades::Memory::DosHeader::SetNewHeaderOffset)
    ;
}

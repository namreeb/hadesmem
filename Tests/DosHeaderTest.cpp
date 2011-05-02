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

#define BOOST_TEST_MODULE DosHeaderTest
#pragma warning(push, 1)
#include <boost/test/unit_test.hpp>
#pragma warning(pop)

#include "HadesMemory/DosHeader.hpp"
#include "HadesMemory/Module.hpp"
#include "HadesMemory/ModuleEnum.hpp"
#include "HadesMemory/PeFile.hpp"
#include "HadesMemory/MemoryMgr.hpp"

BOOST_AUTO_TEST_CASE(BOOST_TEST_MODULE)
{
  Hades::Memory::MemoryMgr MyMemory(GetCurrentProcessId());
    
  for (Hades::Memory::ModuleIter ModIter(MyMemory); *ModIter; ++ModIter)
  {
    Hades::Memory::Module const Mod = **ModIter;
      
    // Todo: Also test FileType_Data
    Hades::Memory::PeFile MyPeFile(MyMemory, Mod.GetBase());
    
    auto HdrRaw = MyMemory.Read<IMAGE_DOS_HEADER>(MyPeFile.GetBase());
    
    Hades::Memory::DosHeader MyDosHdr(MyPeFile);
    BOOST_CHECK_EQUAL(MyDosHdr.IsMagicValid(), true);
    MyDosHdr.EnsureMagicValid();
    MyDosHdr.SetMagic(MyDosHdr.GetMagic());
    MyDosHdr.SetBytesOnLastPage(MyDosHdr.GetBytesOnLastPage());
    MyDosHdr.SetPagesInFile(MyDosHdr.GetPagesInFile());
    MyDosHdr.SetRelocations(MyDosHdr.GetRelocations());
    MyDosHdr.SetSizeOfHeaderInParagraphs(MyDosHdr.GetSizeOfHeaderInParagraphs());
    MyDosHdr.SetMinExtraParagraphs(MyDosHdr.GetMinExtraParagraphs());
    MyDosHdr.SetMaxExtraParagraphs(MyDosHdr.GetMaxExtraParagraphs());
    MyDosHdr.SetInitialSS(MyDosHdr.GetInitialSS());
    MyDosHdr.SetInitialSP(MyDosHdr.GetInitialSP());
    MyDosHdr.SetChecksum(MyDosHdr.GetChecksum());
    MyDosHdr.SetInitialIP(MyDosHdr.GetInitialIP());
    MyDosHdr.SetInitialCS(MyDosHdr.GetInitialCS());
    MyDosHdr.SetRelocTableFileAddr(MyDosHdr.GetRelocTableFileAddr());
    MyDosHdr.SetOverlayNum(MyDosHdr.GetOverlayNum());
    MyDosHdr.SetReservedWords1(MyDosHdr.GetReservedWords1());
    MyDosHdr.SetOEMID(MyDosHdr.GetOEMID());
    MyDosHdr.SetOEMInfo(MyDosHdr.GetOEMInfo());
    MyDosHdr.SetReservedWords2(MyDosHdr.GetReservedWords2());
    MyDosHdr.SetNewHeaderOffset(MyDosHdr.GetNewHeaderOffset());
      
    auto HdrRawNew = MyMemory.Read<IMAGE_DOS_HEADER>(MyPeFile.GetBase());
    
    BOOST_CHECK_EQUAL(std::memcmp(&HdrRaw, &HdrRawNew, sizeof(
      IMAGE_DOS_HEADER)), 0);
  }
}

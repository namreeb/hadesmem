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

#pragma once

namespace Hades
{
  namespace Memory
  {
    // Hades (External)
    class Module;
    class ModuleList;
    class ModuleIter;
    class Region;
    class RegionList;
    class RegionIter;
    class Scanner;
    class Process;
    class Injector;
    class MemoryMgr;
    class ManualMap;
    class FindPattern;
    class Disassembler;
    class Symbols;
    class AllocAndFree;
    
    // Hades (Internal)
    class Patch;
    class PatchRaw;
    class PatchDetour;
    
    // Hades (PeLib)
    class PeFile;
    class TlsDir;
    class Section;
    class SectionList;
    class SectionIter;
    class DosHeader;
    class Export;
    class ExportDir;
    class ExportList;
    class ExportIter;
    class NtHeaders;
    class ImportDir;
    class ImportDirList;
    class ImportDirIter;
    class ImportThunk;
    class ImportThunkList;
    class ImportThunkIter;
  }
}

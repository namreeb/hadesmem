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

// Windows API
#include <crtdbg.h>
#include <Windows.h>

// C++ Standard Library
#include <string>
#include <vector>
#include <iostream>

// Boost
#pragma warning(push, 1)
#pragma warning (disable: ALL_CODE_ANALYSIS_WARNINGS)
#include <boost/python.hpp>
#include <boost/exception/all.hpp>
#pragma warning(pop)

// Hades
#include "PeFileWrap.h"
#include "ModuleWrap.h"
#include "RegionWrap.h"
#include "TlsDirWrap.h"
#include "ScannerWrap.h"
#include "SectionWrap.h"
#include "InjectorWrap.h"
#include "NtHeadersWrap.h"
#include "ExportDirWrap.h"
#include "ImportDirWrap.h"
#include "DosHeaderWrap.h"
#include "MemoryMgrWrap.h"
#include "ManualMapWrap.h"
#include "FindPatternWrap.h"
#include "DisassemblerWrap.h"
#include "Hades-Common/Error.h"
#include "Hades-Memory/AutoLink.h"

// Custom error translator
void HadesErrorTranslator(std::exception const& e)
{
  PyErr_SetString(PyExc_RuntimeError, boost::diagnostic_information(e).
    c_str());
}

#if defined(_M_AMD64) 
BOOST_PYTHON_MODULE(PyHadesMem_AMD64)
#elif defined(_M_IX86) 
BOOST_PYTHON_MODULE(PyHadesMem_IA32)
#else 
#error "Unsupported architecture."
#endif
{
  boost::python::register_exception_translator<std::exception>(
    &HadesErrorTranslator);

  boost::python::class_<std::vector<DWORD_PTR>>("PointerVec", 
    boost::python::no_init)
    .def("__iter__", boost::python::iterator<std::vector<DWORD_PTR>>())
    ;

  ExportDisassembler();
  ExportFindPattern();
  ExportInjector();
  ExportManualMap();
  ExportMemoryMgr();
  ExportModule();
  ExportRegion();
  ExportScanner();

  ExportDosHeader();
  ExportExportDir();
  ExportImportDir();
  ExportNtHeaders();
  ExportPeFile();
  ExportSection();
  ExportTlsDir();
}

BOOL WINAPI DllMain(HINSTANCE /*hinstDLL*/, DWORD /*fdwReason*/, 
  LPVOID /*lpvReserved*/)
{
  // Attempt to detect memory leaks in debug mode
#ifdef _DEBUG
  int CurrentFlags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
  int NewFlags = (_CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_LEAK_CHECK_DF | 
    _CRTDBG_CHECK_ALWAYS_DF);
  _CrtSetDbgFlag(CurrentFlags | NewFlags);
#endif

  return TRUE;
}

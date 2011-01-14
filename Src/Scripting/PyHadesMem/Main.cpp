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
#include <cmath> // GCC workaround
#include <string>
#include <vector>
#include <iostream>

// Boost
#ifdef _MSC_VER
#pragma warning(push, 1)
#endif // #ifdef _MSC_VER
#include <boost/python.hpp>
#include <boost/exception/all.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif // #ifdef _MSC_VER

// Hades
#include "PeFileWrap.hpp"
#include "ModuleWrap.hpp"
#include "RegionWrap.hpp"
#include "TlsDirWrap.hpp"
#include "SymbolWrap.hpp"
#include "ScannerWrap.hpp"
#include "SectionWrap.hpp"
#include "ProcessWrap.hpp"
#include "InjectorWrap.hpp"
#include "NtHeadersWrap.hpp"
#include "ExportDirWrap.hpp"
#include "ImportDirWrap.hpp"
#include "DosHeaderWrap.hpp"
#include "MemoryMgrWrap.hpp"
#include "ManualMapWrap.hpp"
#include "FindPatternWrap.hpp"
#include "DisassemblerWrap.hpp"
#include "HadesCommon/Error.hpp"

// Custom error translator
void HadesErrorTranslator(std::exception const& e)
{
  PyErr_SetString(PyExc_RuntimeError, boost::diagnostic_information(e).
    c_str());
}

BOOST_PYTHON_MODULE(PyHadesMem)
{
  boost::python::register_exception_translator<std::exception>(
    &HadesErrorTranslator);

  boost::python::class_<std::vector<DWORD_PTR>>("PointerVec", 
    boost::python::no_init)
    .def("__iter__", boost::python::iterator<std::vector<DWORD_PTR>>())
    ;

  boost::python::class_<std::vector<std::basic_string<TCHAR>>>("PointerStr", 
    boost::python::no_init)
    .def("__iter__", boost::python::iterator<std::vector<
      std::basic_string<TCHAR>>>())
    ;

  ExportDisassembler();
  ExportFindPattern();
  ExportInjector();
  ExportManualMap();
  ExportMemoryMgr();
  ExportModule();
  ExportProcess();
  ExportRegion();
  ExportScanner();
  ExportSymbol();

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

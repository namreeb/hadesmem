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
#include "HadesMemory/Process.hpp"
    
Hades::Memory::Process CreateProcessWrap(std::wstring const& Path, 
  std::wstring const& Params, 
  std::wstring const& WorkingDir)
{
  return Hades::Memory::CreateProcess(Path, Params, WorkingDir);
}

class ProcessWrap : public Hades::Memory::Process
{
public:
  explicit ProcessWrap(DWORD ProcID) 
    : Hades::Memory::Process(ProcID)
  { }

  explicit ProcessWrap(std::wstring const& ProcName) 
    : Hades::Memory::Process(ProcName)
  { }

  ProcessWrap(std::wstring const& WindowName, 
    std::wstring const& ClassName) 
    : Hades::Memory::Process(WindowName, ClassName)
  { }

  DWORD_PTR GetHandle() const
  {
    return reinterpret_cast<DWORD_PTR>(Hades::Memory::Process::GetHandle());
  }

  std::wstring GetPath() const
  {
    return Hades::Memory::Process::GetPath().native();
  }
};

struct ProcessIterWrap
{
  static ProcessWrap next(Hades::Memory::ProcessIter& o)
  {
    if (!*o)
    {
      PyErr_SetString(PyExc_StopIteration, "No more data.");
      boost::python::throw_error_already_set();
    }

    auto MyProcessWrap(*static_cast<ProcessWrap*>(&**o));

    ++o;

    return MyProcessWrap;
  }

  static boost::python::object pass_through(boost::python::object const& o) 
  {
    return o;
  }

  static void wrap(const char* python_name)
  {
    boost::python::class_<Hades::Memory::ProcessIter, boost::noncopyable>(
      python_name)
      .def("next", next)
      .def("__iter__", pass_through)
      ;
  }
};

// Export Process API
void ExportProcess()
{
  boost::python::def("CreateProcess", &CreateProcessWrap);
  
  boost::python::class_<Hades::Memory::Process>("ProcessBase", 
    boost::python::no_init)
    ;

  boost::python::class_<ProcessWrap, boost::python::bases<Hades::Memory::
    Process>>("Process", boost::python::init<DWORD>())
    .def(boost::python::init<std::wstring const&>())
    .def(boost::python::init<std::wstring const&, 
      std::wstring const&>())
    .def("GetHandle", &ProcessWrap::GetHandle)
    .def("GetID", &ProcessWrap::GetID)
    .def("GetPath", &ProcessWrap::GetPath)
    ;

  ProcessIterWrap::wrap("ProcessIter"); 
}

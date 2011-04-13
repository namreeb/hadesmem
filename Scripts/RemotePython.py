# This file is part of HadesMem.
# Copyright (C) 2011 Joshua Boyce (a.k.a. RaptorFactor).
# <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>
# 
# HadesMem is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# HadesMem is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with HadesMem.  If not, see <http://www.gnu.org/licenses/>.

from sys import version_info

ProcName = raw_input("Process name: ")
MyMem = PyHadesMem.MemoryMgr(ProcName)
PythonDLLName = "python%s.dll"  % ''.join ((str(i) for i in version_info[:2]))
print("Python DLL Name: " + PythonDLLName)
MyInjector = PyHadesMem.Injector(MyMem)
PythonDLLRemote = MyInjector.InjectDll(PythonDLLName, False)
print("Python Module Base: " + hex(PythonDLLRemote))
print("Calling Py_Initialize in remote process")
MyInjector.CallExport(PythonDLLName, PythonDLLRemote, "Py_Initialize")
print("Finding PyRun_SimpleString in remote process")
PyRun_SimpleStringRemote = MyMem.GetRemoteProcAddress(PythonDLLRemote, PythonDLLName, "PyRun_SimpleString")
Command = raw_input("Command: ")
print("Writing command string to remote process")
CmdRemote = MyMem.Alloc(len(Command) + 1)
MyMem.WriteStringA(CmdRemote, Command)
print("Running command in remote process via PyRun_SimpleString")
RunCommandRet = MyMem.Call(PyRun_SimpleStringRemote, [CmdRemote], PyHadesMem.MemoryMgr.CallConv.Default)
print("PyRun_SimpleString Ret: " + str(RunCommandRet))
print("Freeing string in remote process")
MyMem.Free(CmdRemote)
print("Calling Py_Finalize in remote process")
MyInjector.CallExport(PythonDLLName, PythonDLLRemote, "Py_Finalize")

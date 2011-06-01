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
print("Attempting to allocate console in remote process")
AllocConsoleCmd = r"""
import win32console
import code
import sys

class IOWrapper:
  def __init__ (self, hin, hout):
    self.hin = hin
    self.hout = hout
        
  def write(self, data):
    self.hout.WriteConsole(data)
        
  def read(self, L):
    data = self.hin.ReadConsole(L)
    if data == "\r":
      data =self.hin.ReadConsole(1)
    if not data:
      raise SystemExit  
    return data
    
  def readline(self):
    data = []
    while True:
      d = self.read(1)
      if not d:
        raise SystemExit
      data.append (d)
      if d =="\n":
        return "".join (data)

try:
  win32console.AllocConsole()
except:
  pass

hin = win32console.GetStdHandle(win32console.STD_INPUT_HANDLE)
hout = win32console.GetStdHandle(win32console.STD_OUTPUT_HANDLE)

MyWrapper = IOWrapper(hin, hout)
sys.stdout = MyWrapper
sys.stdin = MyWrapper
sys.stderr = MyWrapper

code.interact()
"""
print("Writing console alloc command string to remote process")
AllocConsoleRemote = MyMem.Alloc(len(AllocConsoleCmd) + 1)
MyMem.WriteStringA(AllocConsoleRemote, AllocConsoleCmd)
print("Running console alloc command in remote process via PyRun_SimpleString")
AllocConsoleRet = MyMem.Call(PyRun_SimpleStringRemote, [AllocConsoleRemote], PyHadesMem.MemoryMgr.CallConv.Default)
# Note: Detatching interactive Python instance from remote process without killing process is currently unsupported.
#print("AllocConsole Ret: " + str(AllocConsoleRet))
#print("Freeing console alloc string in remote process")
#MyMem.Free(AllocConsoleRemote)
#print("Calling Py_Finalize in remote process")
#MyInjector.CallExport(PythonDLLName, PythonDLLRemote, "Py_Finalize")

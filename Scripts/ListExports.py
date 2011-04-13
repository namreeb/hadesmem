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

ProcName = raw_input("Process name: ")
MyMem = PyHadesMem.MemoryMgr(ProcName)
ModIter = PyHadesMem.ModuleIter(MyMem)
for MyMod in ModIter:
  print("")
  print("Base: " + hex(MyMod.GetBase()))
  print("Size: " + hex(MyMod.GetSize()))
  print("Name: " + MyMod.GetName())
  print("Path: " + MyMod.GetPath())
  
  MyPeFile = PyHadesMem.PeFile(MyMem, MyMod.GetBase(), PyHadesMem.PeFile.FileType.Image)
  ExpDir = PyHadesMem.ExportDir(MyPeFile)
  if not ExpDir.IsValid():
    continue
  ExpIter = PyHadesMem.ExportIter(MyPeFile)
  for MyExp in ExpIter:
    if MyExp.ByName():
      print(MyMod.GetName() + "!" + MyExp.GetName() + ": " + hex(MyExp.GetRva()))
    else:
      print(MyMod.GetName() + "!" + hex(MyExp.GetOrdinal()) + ": " + hex(MyExp.GetRva()))

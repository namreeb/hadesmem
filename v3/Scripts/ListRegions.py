# This file is part of HadesMem.
# Copyright (C) 2010 Joshua Boyce (aka RaptorFactor, Cypherjb, Cypher, Chazwazza).
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
RegionIter = PyHadesMem.RegionIter(MyMem)
for MyRegion in RegionIter:
  print("")
  print("Base: " + hex(MyRegion.GetBase()))
  print("AllocBase: " + hex(MyRegion.GetAllocBase()))
  print("AllocProtect: " + hex(MyRegion.GetAllocProtect()))
  print("Size: " + hex(MyRegion.GetSize()))
  print("State: " + hex(MyRegion.GetState()))
  print("Protect: " + hex(MyRegion.GetProtect()))
  print("Type: " + hex(MyRegion.GetType()))

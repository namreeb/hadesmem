ProcName = raw_input("Process name: ")
MyMem = PyHadesMem.MemoryMgr(ProcName)
ModIter = PyHadesMem.ModuleIter(MyMem)
for MyMod in ModIter:
  print("")
  print("Base: " + hex(MyMod.GetBase()))
  print("Size: " + hex(MyMod.GetSize()))
  print("Name: " + MyMod.GetName())
  print("Path: " + MyMod.GetPath())

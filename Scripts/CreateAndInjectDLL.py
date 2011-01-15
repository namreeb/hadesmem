ProcPath = raw_input("Process path: ")
WorkingDir = raw_input("Process working dir (optional): ")
ProcArgs = raw_input("Process arguments (optional): ")
ModPath = raw_input("Module path: ")
ExpName = raw_input("Export name (optional): ")
InjectData = PyHadesMem.CreateAndInject(ProcPath, WorkingDir, ProcArgs, 
  ModPath, ExpName)
MyMem = InjectData[0]
ModBase = InjectData[1]
ExpRet = InjectData[2]
print("Module Base: " + hex(ModBase))
if ExpName:
  print("Export Ret: " + hex(ExpRet) + " " + str(ExpRet))

ProcIter = PyHadesMem.ProcessIter()
for MyProc in ProcIter:
  print("")
  print("ID: " + str(MyProc.GetID()))
  print("Path: " + MyProc.GetPath())

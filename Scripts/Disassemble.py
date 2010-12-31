ProcName = raw_input("Process name: ")
MyMem = PyHadesMem.MemoryMgr(ProcName)
MyDisassembler = PyHadesMem.Disassembler(MyMem)
Address = int(raw_input("Target address: "), 16)
Instructions = int(raw_input("Number of instructions: "))
Data = MyDisassembler.DisassembleToStr(Address, Instructions)
for i in Data:
  print(i)

MyMem = HadesMem.CreateMemoryMgr("Steam.exe");

HadesMem.WriteLn("Process ID: " .. MyMem:GetProcessID());

HadesMem.WriteLn("");
SteamMod = HadesMem.CreateModule(MyMem, "Steam.exe");
HadesMem.WriteLn("Steam Base: " .. HadesMem.ToHexStr(SteamMod:GetBase()));
HadesMem.WriteLn("Steam Path: " .. SteamMod:GetPath());

HadesMem.WriteLn("");
HadesMem.WriteLn("Disassembling code (20 instructions) @ Steam.exe+0x00001010.");
MyDisassembler = HadesMem.CreateDisassembler(MyMem);
MyCode = MyDisassembler:DisassembleToStr(SteamMod:GetBase() + 0x00001010, 20);
for CurCode in MyCode.List do
	HadesMem.WriteLn(CurCode);
end

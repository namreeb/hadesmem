MyMem = HadesMem.CreateMemoryMgr("Steam.exe");

HadesMem.WriteLn("Process ID: " .. MyMem:GetProcessID());

HadesMem.WriteLn("");
SteamMod = HadesMem.CreateModule(MyMem, "Steam.exe");
HadesMem.WriteLn("Steam Base: " .. HadesMem.ToHexStr(SteamMod:GetBase()));
HadesMem.WriteLn("Steam Path: " .. SteamMod:GetPath());

HadesMem.WriteLn("");
NtdllMod = HadesMem.CreateModule(MyMem, "ntdll.dll");
HadesMem.WriteLn("NTDLL Base: " .. HadesMem.ToHexStr(NtdllMod:GetBase()));
HadesMem.WriteLn("NTDLL Path: " .. NtdllMod:GetPath());

HadesMem.WriteLn("");
HadesMem.WriteLn("Dumping module list.");
MyModules = HadesMem.GetModuleList(MyMem);
for CurMod in MyModules.List do
	HadesMem.WriteLn("Base: " .. HadesMem.ToHexStr(CurMod:GetBase()) .. ", Name: \"" .. CurMod:GetName() .. "\".");
end

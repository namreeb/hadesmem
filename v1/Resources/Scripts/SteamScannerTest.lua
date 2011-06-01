MyMem = HadesMem.CreateMemoryMgr("Steam.exe");

HadesMem.WriteLn("Process ID: " .. MyMem:GetProcessID());

MyScanner = HadesMem.CreateScanner(MyMem);

HadesMem.WriteLn("");
HadesMem.WriteLn("Scan for all '0xFF6A006A'.");
Int3List = MyScanner:FindAllPointer(0xFF6A006A);
for CurInt3 in Int3List.List do
	HadesMem.WriteLn("Current: " .. HadesMem.ToHexStr(CurInt3));
end
HadesMem.WriteLn("Scan for 'Steam [Release]': " .. HadesMem.ToHexStr(MyScanner:FindStrNarrow("Steam [Release]")));
HadesMem.WriteLn("Scan for '0.64278764': " .. HadesMem.ToHexStr(MyScanner:FindFloat(0.64278764)));
HadesMem.WriteLn("Loading SteamPatterns.xml.");
MyScanner:LoadFromXML("C:/Users/Josh/Documents/visual studio 2010/Projects/HadesMem/Binaries/Full-Debug/Scripts/SteamPatterns.xml");
HadesMem.WriteLn("Pattern MaxClimbAngle: " .. HadesMem.ToHexStr(MyScanner:GetAddress("MaxClimbAngle")));

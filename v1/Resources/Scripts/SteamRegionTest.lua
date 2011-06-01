MyMem = HadesMem.CreateMemoryMgr("Steam.exe");

HadesMem.WriteLn("Process ID: " .. MyMem:GetProcessID());

HadesMem.WriteLn("");
HadesMem.WriteLn("Dumping region list.");
RegionList = HadesMem.GetRegionList(MyMem);
for MyRegion in RegionList.List do
	HadesMem.WriteLn("Base: " .. HadesMem.ToHexStr(MyRegion:GetBase()) .. ", Size: " .. HadesMem.ToHexStr(MyRegion:GetSize()) .. ".");
end

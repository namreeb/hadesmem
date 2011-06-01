MyMem = HadesMem.CreateMemoryMgr("Taskmgr.exe");

HadesMem.WriteLn("Process ID: " .. MyMem:GetProcessID());

MyManualMap = HadesMem.CreateManualMap(MyMem);

HadesMem.WriteLn("");
if HadesMem.IsAMD64() then
	HadesMem.WriteLn("Manually Mapping Hades-MMTester AMD64.");	
	HadesMem.WriteLn("Hades-MMTester Base: " .. HadesMem.ToHexStr(MyManualMap:Map("Hades-MMTester_AMD64.dll", "Initialize", false)));
else
	HadesMem.WriteLn("Manually Mapping Hades-MMTester IA32.");	
	HadesMem.WriteLn("Hades-MMTester Base: " .. HadesMem.ToHexStr(MyManualMap:Map("Hades-MMTester_IA32.dll", "_Initialize@4", false)));
end

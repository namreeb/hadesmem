--MyMem = HadesMem.CreateMemoryMgr("WoW.exe");

--HadesMem.WriteLn("Process ID: " .. MyMem:GetProcessID());

--HadesMem.WriteLn("");
--HadesMem.WriteLn("Injecting Hades-Kernel.");
--MyInjector = HadesMem.CreateInjector(MyMem);
--HadesKernelBase = MyInjector:InjectDll("Hades-Kernel_IA32.dll");
--HadesMem.WriteLn("Hades-Kernel Base: " .. HadesMem.ToHexStr(HadesKernelBase));
--HadesMem.WriteLn("Calling Initialize in Hades-Kernel.");
--InitializeRet = MyInjector:CallExport("Hades-Kernel_IA32.dll", HadesKernelBase, "_Initialize@4");
--HadesMem.WriteLn("Hades-Kernel Initialize: " .. InitializeRet);

MyInjectInfo = HadesMem.CreateAndInject("C:/Users/Public/Games/World of Warcraft/Launcher.exe", "", "Hades-Kernel_IA32.dll", "_Initialize@4");
MyMem = MyInjectInfo.Memory;

HadesMem.WriteLn("");
HadesMem.WriteLn("Process ID: " .. MyMem:GetProcessID());
HadesMem.WriteLn("Hades-Kernel Base: " .. HadesMem.ToHexStr(MyInjectInfo.ModBase));
HadesMem.WriteLn("Hades-Kernel Initialize: " .. MyInjectInfo.ExportRet);

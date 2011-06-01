MyMem = HadesMem.CreateMemoryMgr("steam.exe");

HadesMem.WriteLn("Process ID: " .. MyMem:GetProcessID());

HadesMem.WriteLn("");
HadesMem.WriteLn("Process Handle: " .. HadesMem.ToHexStr(MyMem:GetProcessHandle()));

HadesMem.WriteLn("");
SteamMod = HadesMem.CreateModule(MyMem, "Steam.exe");
HadesMem.WriteLn("Steam Base: " .. HadesMem.ToHexStr(SteamMod:GetBase()));
HadesMem.WriteLn("Steam Path: " .. SteamMod:GetPath());

HadesMem.WriteLn("");
Kernel32Mod = HadesMem.CreateModule(MyMem, "kernel32.dll");
HadesMem.WriteLn("Kernel32 Base: " .. HadesMem.ToHexStr(Kernel32Mod:GetBase()));
HadesMem.WriteLn("Kernel32 Path: " .. Kernel32Mod:GetPath());

HadesMem.WriteLn("");
EncodePointerRem = MyMem:GetRemoteProcAddress(Kernel32Mod:GetBase(), "kernel32.dll", "EncodePointer");
HadesMem.WriteLn("kernel32!EncodePointer: " .. HadesMem.ToHexStr(EncodePointerRem));
EncodePointerArgs = std.vector_dwordptr();
EncodePointerArgs:push_back(0x13371337);
EncodePointerRet = MyMem:Call(EncodePointerRem, EncodePointerArgs, HadesMem.MemoryMgr.CallConv_STDCALL);
HadesMem.WriteLn("Result: " .. HadesMem.ToHexStr(EncodePointerRet));
DecodePointerRem = MyMem:GetRemoteProcAddress(Kernel32Mod:GetBase(), "kernel32.dll", "DecodePointer");
HadesMem.WriteLn("kernel32!DecodePointer: " .. HadesMem.ToHexStr(DecodePointerRem));
DecodePointerArgs = std.vector_dwordptr();
DecodePointerArgs:push_back(EncodePointerRet);
HadesMem.WriteLn("Result: " .. HadesMem.ToHexStr(MyMem:Call(DecodePointerRem, DecodePointerArgs, HadesMem.MemoryMgr.CallConv_STDCALL)));

HadesMem.WriteLn("");
ReadTarget = SteamMod:GetBase() + 0x00001001;
HadesMem.WriteLn("Int8 @ Steam.exe+0x00001001 (Pre): " .. MyMem:ReadInt8(ReadTarget));
MyMem:WriteInt8(ReadTarget, 5);
HadesMem.WriteLn("Int8 @ Steam.exe+0x00001001 (Post): " .. MyMem:ReadInt8(ReadTarget));
HadesMem.WriteLn("UInt8 @ Steam.exe+0x00001001 (Pre): " .. MyMem:ReadUInt8(ReadTarget));
MyMem:WriteUInt8(ReadTarget, 10);
HadesMem.WriteLn("UInt8 @ Steam.exe+0x00001001 (Post): " .. MyMem:ReadUInt8(ReadTarget));
HadesMem.WriteLn("Int16 @ Steam.exe+0x00001001 (Pre): " .. MyMem:ReadInt16(ReadTarget));
MyMem:WriteInt16(ReadTarget, 15);
HadesMem.WriteLn("Int16 @ Steam.exe+0x00001001 (Post): " .. MyMem:ReadInt16(ReadTarget));
HadesMem.WriteLn("UInt16 @ Steam.exe+0x00001001 (Pre): " .. MyMem:ReadUInt16(ReadTarget));
MyMem:WriteUInt16(ReadTarget, 20);
HadesMem.WriteLn("UInt16 @ Steam.exe+0x00001001 (Post): " .. MyMem:ReadUInt16(ReadTarget));
HadesMem.WriteLn("Int32 @ Steam.exe+0x00001001 (Pre): " .. MyMem:ReadInt32(ReadTarget));
MyMem:WriteInt32(ReadTarget, 25);
HadesMem.WriteLn("Int32 @ Steam.exe+0x00001001 (Post " .. MyMem:ReadInt32(ReadTarget));
HadesMem.WriteLn("UInt32 @ Steam.exe+0x00001001 (Pre): " .. MyMem:ReadUInt32(ReadTarget));
MyMem:WriteUInt32(ReadTarget, 30);
HadesMem.WriteLn("UInt32 @ Steam.exe+0x00001001 (Post): " .. MyMem:ReadUInt32(ReadTarget));
--HadesMem.WriteLn("Int64 @ Steam.exe+0x00001001 (Pre): " .. MyMem:ReadInt64(ReadTarget));
--HadesMem.WriteLn("Int64 @ Steam.exe+0x00001001 (Post): " .. MyMem:ReadInt64(ReadTarget));
--HadesMem.WriteLn("UInt64 @ Steam.exe+0x00001001 (Pre): " .. MyMem:ReadUInt64(ReadTarget));
--HadesMem.WriteLn("UInt64 @ Steam.exe+0x00001001 (Post): " .. MyMem:ReadUInt64(ReadTarget));
HadesMem.WriteLn("Float @ Steam.exe+0x00001001 (Pre): " .. MyMem:ReadFloat(ReadTarget));
MyMem:WriteFloat(ReadTarget, 35.1);
HadesMem.WriteLn("Float @ Steam.exe+0x00001001 (Post): " .. MyMem:ReadFloat(ReadTarget));
HadesMem.WriteLn("Double @ Steam.exe+0x00001001 (Pre): " .. MyMem:ReadDouble(ReadTarget));
MyMem:WriteDouble(ReadTarget, 40.1);
HadesMem.WriteLn("Double @ Steam.exe+0x00001001 (Post): " .. MyMem:ReadDouble(ReadTarget));
HadesMem.WriteLn("CharNarrow @ Steam.exe+0x00001001 (Pre): " .. MyMem:ReadCharNarrow(ReadTarget));
MyMem:WriteCharNarrow(ReadTarget, "a");
HadesMem.WriteLn("CharNarrow @ Steam.exe+0x00001001 (Post): " .. MyMem:ReadCharNarrow(ReadTarget));
HadesMem.WriteLn("CharWide @ Steam.exe+0x00001001 (Pre): " .. MyMem:ReadCharWide(ReadTarget));
MyMem:WriteCharWide(ReadTarget, "b");
HadesMem.WriteLn("CharWide @ Steam.exe+0x00001001 (Post): " .. MyMem:ReadCharWide(ReadTarget));
HadesMem.WriteLn("StrNarrow @ Steam.exe+0x00001001 (Pre): " .. MyMem:ReadStrNarrow(ReadTarget));
MyMem:WriteStrNarrow(ReadTarget, "1234");
HadesMem.WriteLn("StrNarrow @ Steam.exe+0x00001001 (Post): " .. MyMem:ReadStrNarrow(ReadTarget));
HadesMem.WriteLn("StrWide @ Steam.exe+0x00001001 (Pre): " .. MyMem:ReadStrWide(ReadTarget));
MyMem:WriteStrWide(ReadTarget, "5678");
HadesMem.WriteLn("StrWide @ Steam.exe+0x00001001 (Post): " .. MyMem:ReadStrWide(ReadTarget));
HadesMem.WriteLn("Pointer @ Steam.exe+0x00001001 (Pre): " .. HadesMem.ToHexStr(MyMem:ReadPointer(ReadTarget)));
MyMem:WritePointer(ReadTarget, 0x13371337);
HadesMem.WriteLn("Pointer @ Steam.exe+0x00001001 (Post): " .. HadesMem.ToHexStr(MyMem:ReadPointer(ReadTarget)));

HadesMem.WriteLn("");
CanReadTarget = MyMem:CanRead(ReadTarget) and "true" or "false";
HadesMem.WriteLn("CanRead @ Steam.exe+0x00001001: " .. CanReadTarget);
CanWriteTarget = MyMem:CanWrite(ReadTarget) and "true" or "false";
HadesMem.WriteLn("CanWrite @ Steam.exe+0x00001001: " .. CanWriteTarget);
AllocMem = MyMem:Alloc(0x1000);
HadesMem.WriteLn("Alloc (0x1000): " .. HadesMem.ToHexStr(AllocMem));
HadesMem.WriteLn("Freeing allocated memory.");
MyMem:Free(AllocMem);

HadesMem.WriteLn("");
HadesMem.WriteLn("FlushCache @ Steam.exe+0x00001001.");
MyMem:FlushCache(ReadTarget, 10);

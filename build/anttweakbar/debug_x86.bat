set OLDCD=%CD%
pushd ..\..\src\anttweakbar\anttweakbar
md obj\debug32
fxc /Od /Zi /T vs_4_0_level_9_1 /E LineRectVS /Fh obj\debug32\TwDirect3D11_LineRectVS.h src\TwDirect3D11.hlsl > %OLDCD%\debug_x86.txt 2>&1
fxc /Od /Zi /T vs_4_0_level_9_1 /E LineRectCstColorVS /Fh obj\debug32\TwDirect3D11_LineRectCstColorVS.h src\TwDirect3D11.hlsl >> %OLDCD%\debug_x86.txt 2>&1
fxc /Od /Zi /T ps_4_0_level_9_1 /E LineRectPS /Fhobj\debug32\TwDirect3D11_LineRectPS.h src\TwDirect3D11.hlsl >> %OLDCD%\debug_x86.txt 2>&1
fxc /Od /Zi /T vs_4_0_level_9_1 /E TextVS /Fh obj\debug32\TwDirect3D11_TextVS.h src\TwDirect3D11.hlsl >> %OLDCD%\debug_x86.txt 2>&1
fxc /Od /Zi /T vs_4_0_level_9_1 /E TextCstColorVS /Fh obj\debug32\TwDirect3D11_TextCstColorVS.h src\TwDirect3D11.hlsl >> %OLDCD%\debug_x86.txt 2>&1
fxc /Od /Zi /T ps_4_0_level_9_1 /E TextPS /Fh obj\debug32\TwDirect3D11_TextPS.h src\TwDirect3D11.hlsl >> %OLDCD%\debug_x86.txt 2>&1
popd

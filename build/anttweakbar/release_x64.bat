set OLDCD=%CD%
pushd ..\..\src\anttweakbar\anttweakbar
md obj\release64
fxc /T vs_4_0_level_9_1 /E LineRectVS /Fh obj\release64\TwDirect3D11_LineRectVS.h src\TwDirect3D11.hlsl > %OLDCD%\release_x64.txt 2>&1
fxc /T vs_4_0_level_9_1 /E LineRectCstColorVS /Fh obj\release64\TwDirect3D11_LineRectCstColorVS.h src\TwDirect3D11.hlsl >> %OLDCD%\release_x64.txt 2>&1
fxc /T ps_4_0_level_9_1 /E LineRectPS /Fhobj\release64\TwDirect3D11_LineRectPS.h src\TwDirect3D11.hlsl >> %OLDCD%\release_x64.txt 2>&1
fxc /T vs_4_0_level_9_1 /E TextVS /Fh obj\release64\TwDirect3D11_TextVS.h src\TwDirect3D11.hlsl >> %OLDCD%\release_x64.txt 2>&1
fxc /T vs_4_0_level_9_1 /E TextCstColorVS /Fh obj\release64\TwDirect3D11_TextCstColorVS.h src\TwDirect3D11.hlsl >> %OLDCD%\release_x64.txt 2>&1
fxc /T ps_4_0_level_9_1 /E TextPS /Fh obj\release64\TwDirect3D11_TextPS.h src\TwDirect3D11.hlsl >> %OLDCD%\release_x64.txt 2>&1
popd

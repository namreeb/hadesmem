set OLDDIR=%CD%
cd ../../
b2 -j 4 toolset=msvc-10.0 address-model=64 debug > Build\Full\Build_MSVC100_Debug_AMD64.txt
cd %OLDDIR%

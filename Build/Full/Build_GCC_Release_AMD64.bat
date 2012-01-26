set PATH=C:\MinGW64\bin;%PATH%
set OLDDIR=%CD%
cd ../../
b2 -j 4 toolset=gcc address-model=64 release > Build\Full\Build_GCC_Release_AMD64.txt
cd %OLDDIR%

set PATH=C:\MinGW32\bin;%PATH%
set OLDDIR=%CD%
cd ../../
b2 -j 4 toolset=gcc address-model=32 debug > Build\Full\Build_GCC_Debug_IA32.txt
cd %OLDDIR%

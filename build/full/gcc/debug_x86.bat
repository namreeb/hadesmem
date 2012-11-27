set OLDCD=%CD%
pushd ..\..\..\
set OLDPATH=%PATH%
set PATH=%BOOST_ROOT%;%MINGW32%;%MINGW32%\bin\;%PATH%
b2 -j %NUMBER_OF_PROCESSORS% toolset=gcc address-model=32 debug > %OLDCD%\debug_x86.txt 2>&1
set PATH=%OLDPATH%
popd

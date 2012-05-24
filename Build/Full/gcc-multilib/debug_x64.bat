set OLDCD=%CD%
pushd ..\..\..\
set OLDPATH=%PATH%
set PATH=%BOOST_ROOT%;%MINGW%;%MINGW%\bin\;%PATH%
b2 -j %NUMBER_OF_PROCESSORS% toolset=gcc address-model=64 debug > %OLDCD%\debug_x64.txt
set PATH=%OLDPATH%
popd

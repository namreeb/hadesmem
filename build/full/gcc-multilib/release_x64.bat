set OLDCD=%CD%
pushd ..\..\..\
set OLDPATH=%PATH%
set PATH=%BOOST_ROOT%;%MINGW%;%MINGW%\bin\;%PATH%
b2 -j %NUMBER_OF_PROCESSORS% toolset=gcc address-model=64 release > %OLDCD%\release_x64.txt 2>&1
set PATH=%OLDPATH%
popd

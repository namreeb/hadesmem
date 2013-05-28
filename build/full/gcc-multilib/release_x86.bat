
set OLDCD=%CD%
pushd ..\..\..\
set OLDPATH=%PATH%
set PATH=%BOOST_ROOT%;%MINGW%;%MINGW%\bin\;%MINGW%\x86_64-w64-mingw32\lib32;%PATH%
b2 -j %NUMBER_OF_PROCESSORS% toolset=gcc address-model=32 release > %OLDCD%\release_x86.txt 2>&1
set PATH=%OLDPATH%
popd

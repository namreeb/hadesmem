set OLDCD=%CD%
pushd ..\..\..\
set OLDPATH=%PATH%
set PATH=%BOOST_ROOT%;%PATH%
b2 -j %NUMBER_OF_PROCESSORS% toolset=msvc address-model=32 release > %OLDCD%\release_x86.txt 2>&1
set PATH=%OLDPATH%
popd

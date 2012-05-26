set OLDCD=%CD%
pushd ..\..\..\
set OLDPATH=%PATH%
set PATH=%BOOST_ROOT%;%PATH%
b2 -j %NUMBER_OF_PROCESSORS% toolset=msvc address-model=64 release > %OLDCD%\release_x64.txt
set PATH=%OLDPATH%
popd

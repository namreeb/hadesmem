set OLDCD=%CD%
pushd ..\..\..\
set OLDPATH=%PATH%
set PATH=%BOOST_ROOT%;%PATH%
b2 -j %NUMBER_OF_PROCESSORS% toolset=intel address-model=64 debug > %OLDCD%\debug_x64.txt
set PATH=%OLDPATH%
popd

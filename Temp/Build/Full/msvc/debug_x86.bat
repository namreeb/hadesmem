set OLDCD=%CD%
pushd ..\..\..\
set OLDPATH=%PATH%
set PATH=%BOOST_ROOT%;%PATH%
b2 -j %NUMBER_OF_PROCESSORS% toolset=msvc address-model=32 debug > %OLDCD%\debug_x86.txt
set PATH=%OLDPATH%
popd

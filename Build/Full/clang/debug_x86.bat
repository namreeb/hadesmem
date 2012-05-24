set OLDCD=%CD%
pushd ..\..\..\
set OLDPATH=%PATH%
set PATH=%BOOST_ROOT%;%MINGW32_CLANG%;%MINGW32_CLANG%\bin\;%PATH%
b2 -j %NUMBER_OF_PROCESSORS% toolset=clang address-model=32 debug > %OLDCD%\debug_x86.txt
set PATH=%OLDPATH%
popd

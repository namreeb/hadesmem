set OLDCD=%CD%
pushd ..\..\
set OLDPATH=%PATH%
set PATH=%BOOST_ROOT%;%MINGW64%;%MINGW64%\bin\;%PATH%
b2 docs -j %NUMBER_OF_PROCESSORS% toolset=gcc address-model=32 release > %OLDCD%\gcc_release_x86.txt
set PATH=%OLDPATH%
popd

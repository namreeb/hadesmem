set OLDCD=%CD%
pushd ..\..\
set OLDPATH=%PATH%
set PATH=%BOOST_ROOT%;%PATH%
b2 docs -j %NUMBER_OF_PROCESSORS% toolset=msvc address-model=32 release > %OLDCD%\msvc_release_x86.txt
set PATH=%OLDPATH%
popd

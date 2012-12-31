set OLDCD=%CD%
pushd %BOOST_ROOT%
set OLDPATH=%PATH%
set PATH=%MINGW32_CLANG%;%MINGW32_CLANG%\bin\;%PATH%
b2 -j %NUMBER_OF_PROCESSORS% --without-python toolset=clang cxxflags=-std=c++11 address-model=32 architecture=x86 --stagedir=stage/clang-x86 link=static runtime-link=static threading=multi debug-symbols=on define=WINVER=_WIN32_WINNT_VISTA define=_WIN32_WINNT=_WIN32_WINNT_VISTA stage > %OLDCD%\clang32.txt 2>&1
set PATH=%OLDPATH%
popd

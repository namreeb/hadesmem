set OLDCD=%CD%
pushd %BOOST_ROOT%
set OLDPATH=%PATH%
set PATH=%MINGW%;%MINGW%\bin\;%PATH%
b2 -j %NUMBER_OF_PROCESSORS% toolset=gcc cxxflags=-std=c++11 address-model=64 architecture=x86 --stagedir=stage/gcc-multi-x64 link=static runtime-link=static threading=multi debug-symbols=on define=WINVER=_WIN32_WINNT_VISTA define=_WIN32_WINNT=_WIN32_WINNT_VISTA stage > %OLDCD%\gcc64-multilib.txt 2>&1
set PATH=%OLDPATH%
popd

set OLDCD=%CD%
pushd %BOOST_ROOT%
set OLDPATH=%PATH%
set PATH=%LLVM_CLANG%;%PATH%
b2 -j %NUMBER_OF_PROCESSORS% --without-python toolset=clang cxxflags=-std=c++11 address-model=32 architecture=x86 variant=release --stagedir=stage/clang-x86 link=static runtime-link=shared threading=multi debug-symbols=on define=STRICT define=STRICT_TYPED_ITEMIDS define=UNICODE define=_UNICODE define=WINVER=_WIN32_WINNT_VISTA define=_WIN32_WINNT=_WIN32_WINNT_VISTA define=BOOST_USE_WINDOWS_H stage > %OLDCD%\release_x86.txt 2>&1
set PATH=%OLDPATH%
popd

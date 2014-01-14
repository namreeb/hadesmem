set OLDCD=%CD%
pushd %BOOST_ROOT%
set OLDPATH=%PATH%
set PATH=%LLVM_CLANG%;%PATH%
b2 headers > %OLDCD%\release_x64.txt 2>&1
b2 -j %NUMBER_OF_PROCESSORS% --with-program_options toolset=clang cxxflags=-std=c++1y address-model=64 architecture=x86 variant=release --stagedir=stage/clang-x64 link=static runtime-link=shared threading=multi debug-symbols=on define=STRICT define=STRICT_TYPED_ITEMIDS define=UNICODE define=_UNICODE define=WINVER=_WIN32_WINNT_VISTA define=_WIN32_WINNT=_WIN32_WINNT_VISTA define=BOOST_USE_WINDOWS_H stage >> %OLDCD%\release_x64.txt 2>&1
set PATH=%OLDPATH%
popd

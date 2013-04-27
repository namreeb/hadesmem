set OLDCD=%CD%
pushd %BOOST_ROOT%
b2 -j %NUMBER_OF_PROCESSORS% --without-python toolset=msvc address-model=64 architecture=x86 --stagedir=stage/msvc-x64 link=static runtime-link=static threading=multi debug-symbols=on define=WINVER=_WIN32_WINNT_VISTA define=_WIN32_WINNT=_WIN32_WINNT_VISTA define=BOOST_USE_WINDOWS_H stage > %OLDCD%\msvc64.txt 2>&1
popd

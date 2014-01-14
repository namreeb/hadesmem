set OLDCD=%CD%
pushd %BOOST_ROOT%
b2 headers > %OLDCD%\debug_x86.txt 2>&1
b2 -j %NUMBER_OF_PROCESSORS% --with-program_options toolset=msvc address-model=32 architecture=x86 variant=debug --stagedir=stage/msvc-x86 link=static runtime-link=shared threading=multi debug-symbols=on define=STRICT define=STRICT_TYPED_ITEMIDS define=UNICODE define=_UNICODE define=WINVER=_WIN32_WINNT_VISTA define=_WIN32_WINNT=_WIN32_WINNT_VISTA define=BOOST_USE_WINDOWS_H stage >> %OLDCD%\debug_x86.txt 2>&1
popd

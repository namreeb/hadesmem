set OLDCD=%CD%
pushd %BOOST_ROOT%
b2 -j %NUMBER_OF_PROCESSORS% --without-python toolset=intel cxxflags=/Qstd=c++0x address-model=64 architecture=x86 --stagedir=stage/intel-x64 link=static runtime-link=shared threading=multi debug-symbols=on define=WINVER=_WIN32_WINNT_VISTA define=_WIN32_WINNT=_WIN32_WINNT_VISTA define=BOOST_USE_WINDOWS_H stage > %OLDCD%\intel64.txt 2>&1
popd

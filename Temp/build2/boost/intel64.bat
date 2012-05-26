set OLDCD=%CD%
pushd %BOOST_ROOT%
b2 -j %NUMBER_OF_PROCESSORS% toolset=intel cxxflags=/Qstd=c++0x address-model=64 architecture=x86 --stagedir=stage/intel-x64 link=static runtime-link=static threading=multi debug-symbols=on define=WINVER=_WIN32_WINNT_VISTA define=_WIN32_WINNT=_WIN32_WINNT_VISTA stage > %OLDCD%\intel64.txt
popd

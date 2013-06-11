set OLDCD=%CD%
pushd %BOOST_ROOT%
b2 -j %NUMBER_OF_PROCESSORS% --without-python toolset=intel cxxflags=/Qstd=c++0x address-model=64 architecture=x86 --stagedir=stage/intel-x64 link=static runtime-link=shared threading=multi debug-symbols=on define=STRICT define=STRICT_TYPED_ITEMIDS define=UNICODE define=_UNICODE define=WINVER=_WIN32_WINNT_VISTA define=_WIN32_WINNT=_WIN32_WINNT_VISTA define=BOOST_USE_WINDOWS_H define=BOOST_FILESYSTEM_NO_DEPRECATED define=BOOST_SYSTEM_NO_DEPRECATED stage > %OLDCD%\intel64.txt 2>&1
popd

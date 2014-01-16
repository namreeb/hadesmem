set OLDCD=%CD%
pushd %BOOST_ROOT%
b2 headers > %OLDCD%\headers.txt 2>&1
popd

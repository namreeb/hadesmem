set OLDCD=%CD%
pushd %BOOST_ROOT%
bootstrap.bat > %OLDCD%\bootstrap.txt 2>&1
popd

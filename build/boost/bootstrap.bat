set OLDCD=%CD%
pushd %BOOST_ROOT%
bootstrap.bat > %OLDCD%\bootstrap.txt
popd

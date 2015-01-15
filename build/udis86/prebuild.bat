set OLDCD=%CD%
pushd ..\..\deps\udis86\udis86
set OLDPATH=%PATH%
set PATH=%PYTHON_ROOT%;%PATH%
python.exe scripts/ud_itab.py docs/x86/optable.xml libudis86/ > %OLDCD%\prebuild.txt 2>&1
set PATH=%OLDPATH%
popd

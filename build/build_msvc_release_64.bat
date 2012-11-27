@call "%VS110COMNTOOLS%\..\..\VC\vcvarsall.bat" amd64 > build_msvc_release_64.txt 2>&1
nmake /f makefile.msvc CFG=release >> build_msvc_release_64.txt 2>&1
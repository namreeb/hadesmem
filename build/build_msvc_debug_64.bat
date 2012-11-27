@call "%VS110COMNTOOLS%\..\..\VC\vcvarsall.bat" amd64 > build_msvc_debug_64.txt 2>&1
nmake /f makefile.msvc CFG=debug >> build_msvc_debug_64.txt 2>&1
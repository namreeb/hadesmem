@call "%VS110COMNTOOLS%\..\..\VC\vcvarsall.bat" x86 > build_msvc_debug_32.txt 2>&1
nmake /f makefile.msvc CFG=debug >> build_msvc_debug_32.txt 2>&1
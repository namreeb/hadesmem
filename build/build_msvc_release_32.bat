@call "%VS110COMNTOOLS%\..\..\VC\vcvarsall.bat" x86 > build_msvc_release_32.txt 2>&1
nmake /f makefile.msvc CFG=release >> build_msvc_release_32.txt 2>&1
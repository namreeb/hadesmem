HADESMEM_SRC=..\..\src\memory\memory
HADESMEM_INC=..\..\include\memory
ASMJIT_INC=..\..\src\asmjit\asmjit\src

debug:
  md debug
  cl /c /Zi /Gm /Fo.\debug\ /Fd.\debug\ /GR /GS /RTC1 /analyze /W4 /WX /EHsc /Od /sdl /fp:precise /Gd /Oy- /MDd /D "WIN32" /D "_DEBUG" /D "_LIB" /D "_UNICODE" /D "UNICODE" /D "_CRT_SECURE_NO_WARNINGS" /D "_SCL_SECURE_NO_WARNINGS" /nologo /D "HADESMEM_MSVC" /D "WINVER=_WIN32_WINNT_VISTA" /D "_WIN32_WINNT=_WIN32_WINNT_VISTA" /D "_VARIADIC_MAX=10" /I $(HADESMEM_INC) /I $(ASMJIT_INC) $(HADESMEM_SRC)\*.cpp $(HADESMEM_SRC)\detail\*.cpp
  lib /nologo /WX /out:debug\memory.lib debug\*.obj

clean:
  del debug\*.obj debug\*.idb debug\*.pdb debug\*.lib
  rd debug

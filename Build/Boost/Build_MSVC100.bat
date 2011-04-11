cd ..\..\Src\Dependencies\Boost
call bootstrap.bat
bjam -j 4 toolset=msvc-10.0 threading=multi link=static runtime-link=shared address-model=32 debug release stage > ..\..\..\Build\Boost\Build_MSVC100_IA32.txt
ren stage\lib ia32
bjam -j 4 toolset=msvc-10.0 threading=multi link=static runtime-link=shared address-model=64 debug release stage > ..\..\..\Build\Boost\Build_MSVC100_AMD64.txt
ren stage\lib amd64
mkdir stage\lib
move stage\amd64 stage\lib
move stage\ia32 stage\lib

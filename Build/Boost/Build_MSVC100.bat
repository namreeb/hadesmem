cd ..\..\Src\Dependencies\Boost
bjam --toolset=msvc-10.0 --build-type=complete -j 4 address-model=32 stage > ..\..\..\Build\Boost\Build_MSVC100_IA32.txt
ren stage\lib ia32
bjam --toolset=msvc-10.0 --build-type=complete -j 4 address-model=64 stage > ..\..\..\Build\Boost\Build_MSVC100_AMD64.txt
ren stage\lib amd64
mkdir stage\lib
move stage\amd64 stage\lib
move stage\ia32 stage\lib

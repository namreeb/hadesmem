cd ..\..\Src\Dependencies\Boost
bjam -j 4 toolset=gcc threading=multi link=static runtime-link=shared address-model=32 cxxflags=-std=c++0x debug release stage > ..\..\..\Build\Boost\Build_GCC_IA32.txt
ren stage\lib gcc_ia32
bjam -j 4 toolset=gcc threading=multi link=static runtime-link=shared address-model=64 cxxflags=-std=c++0x debug release stage > ..\..\..\Build\Boost\Build_GCC_AMD64.txt
ren stage\lib gcc_amd64
mkdir stage\lib
move stage\gcc_amd64 stage\lib
move stage\gcc_ia32 stage\lib

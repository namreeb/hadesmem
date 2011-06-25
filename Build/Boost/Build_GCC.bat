cd ..\..\Src\Dependencies\Boost
bjam -j 4 --stagedir=stage/gcc-x86 toolset=gcc threading=multi link=static runtime-link=shared address-model=32 cxxflags=-std=c++0x debug release > ..\..\..\Build\Boost\Build_GCC_IA32.txt
bjam -j 4 --stagedir=stage/gcc-x64 toolset=gcc threading=multi link=static runtime-link=shared address-model=64 cxxflags=-std=c++0x debug release > ..\..\..\Build\Boost\Build_GCC_AMD64.txt

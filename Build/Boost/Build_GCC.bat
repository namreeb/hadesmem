cd ..\..\Src\Dependencies\Boost
set BOOST_ROOT=%CD%
b2 -j 4 --without-mpi --without-python --stagedir=stage/gcc-x86 toolset=gcc threading=multi link=shared runtime-link=shared address-model=32 cxxflags=-std=c++0x debug release > ..\..\..\Build\Boost\Build_GCC_IA32.txt
b2 -j 4 --without-mpi --without-python --stagedir=stage/gcc-x64 toolset=gcc threading=multi link=shared runtime-link=shared address-model=64 cxxflags=-std=c++0x debug release > ..\..\..\Build\Boost\Build_GCC_AMD64.txt

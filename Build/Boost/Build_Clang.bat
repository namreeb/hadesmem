cd ..\..\Src\Dependencies\Boost
set BOOST_ROOT=%CD%
bjam -j 4 --stagedir=stage/clang-x86 toolset=clang threading=multi link=shared runtime-link=shared address-model=32 cxxflags=-std=c++0x debug release > ..\..\..\Build\Boost\Build_Clang_IA32.txt
bjam -j 4 --stagedir=stage/clang-x64 toolset=clang threading=multi link=shared runtime-link=shared address-model=64 cxxflags=-std=c++0x debug release > ..\..\..\Build\Boost\Build_Clang_AMD64.txt

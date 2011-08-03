cd ..\..\Src\Dependencies\Boost
set BOOST_ROOT=%CD%
b2 -j 4 --with-date_time --with-exception --with-filesystem --with-system --with-test --with-thread --stagedir=stage/clang-x86 toolset=clang threading=multi link=shared link=static runtime-link=shared address-model=32 cxxflags=-std=c++0x debug release > ..\..\..\Build\Boost\Build_Clang_IA32.txt

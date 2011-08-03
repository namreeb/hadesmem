cd ..\..\Src\Dependencies\Boost
set BOOST_ROOT=%CD%
b2 -j 4 --with-date_time --with-exception --with-filesystem --with-system --with-test --with-thread --stagedir=stage/gcc-x64 toolset=gcc threading=multi link=shared link=static runtime-link=shared address-model=64 cxxflags=-std=c++0x debug release > ..\..\..\Build\Boost\Build_GCC_AMD64.txt

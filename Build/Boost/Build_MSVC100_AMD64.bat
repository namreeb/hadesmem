cd ..\..\Src\Dependencies\Boost
set BOOST_ROOT=%CD%
b2 -j 4 --with-exception --with-filesystem --with-system --with-test --with-thread --stagedir=stage/msvc-x64 toolset=msvc-10.0 threading=multi link=shared runtime-link=shared address-model=64 debug release > ..\..\..\Build\Boost\Build_MSVC100_AMD64.txt

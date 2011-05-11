cd ..\..\Src\Dependencies\Boost\status
"../bjam.exe" -j 4 toolset=gcc threading=multi link=static runtime-link=shared address-model=32 cxxflags=-std=c++0x > ..\..\..\..\Build\Boost\Test_GCC_IA32.txt
"../bjam.exe" -j 4 toolset=gcc threading=multi link=static runtime-link=shared address-model=64 cxxflags=-std=c++0x > ..\..\..\..\Build\Boost\Test_GCC_AMD64.txt

cd ..\..\Src\Dependencies\Boost
bjam -j 4 toolset=msvc-10.0 threading=multi link=static runtime-link=shared address-model=32 debug release > ..\..\..\Build\Boost\Build_MSVC100_IA32.txt
bjam -j 4 toolset=msvc-10.0 threading=multi link=static runtime-link=shared address-model=64 debug release > ..\..\..\Build\Boost\Build_MSVC100_AMD64.txt

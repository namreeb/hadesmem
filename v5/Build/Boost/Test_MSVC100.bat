cd ..\..\Src\Dependencies\Boost\status
"../bjam.exe" -j 4 toolset=msvc-10.0 threading=multi link=static runtime-link=shared address-model=32 > ..\..\..\..\Build\Boost\Test_MSVC100_IA32.txt
"../bjam.exe" -j 4 toolset=msvc-10.0 threading=multi link=static runtime-link=shared address-model=64 > ..\..\..\..\Build\Boost\Test_MSVC100_AMD64.txt

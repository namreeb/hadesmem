cd ../../
cd Src/Dependencies/Boost
set BOOST_ROOT=%CD%
cd ../../../
bjam Src/Dependencies/Boost/tools/quickbook/ -j 4 toolset=msvc-10.0 address-model=32 release > Build\Docs\Build_MSVC100_Release_IA32.txt
cd Src/Dependencies/Boost/dist/bin/
copy quickbook.exe ..\..\..\..\..\Docs\Bin\bin\quickbook.exe
cd ../../../../../
bjam Docs -j 4 toolset=msvc-10.0 address-model=32 release >> Build\Docs\Build_MSVC100_Release_IA32.txt

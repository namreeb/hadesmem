cd ../../
cd Src/Dependencies/Boost
set BOOST_ROOT=%CD%
cd ../../../
b2 -j 4 toolset=clang address-model=32 debug > Build\Full\Build_Clang_Debug_IA32.txt

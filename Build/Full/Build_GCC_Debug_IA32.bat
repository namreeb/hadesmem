cd ../../
cd Src/Dependencies/Boost
set BOOST_ROOT=%CD%
cd ../../../
b2 -j 4 toolset=gcc address-model=32 debug > Build\Full\Build_GCC_Debug_IA32.txt

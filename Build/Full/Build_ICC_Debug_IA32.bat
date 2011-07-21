cd ../../
cd Src/Dependencies/Boost
set BOOST_ROOT=%CD%
cd ../../../
b2 -j 4 toolset=intel address-model=32 debug > Build\Full\Build_ICC_Debug_IA32.txt

cd ../../
cd Src/Dependencies/Boost
set BOOST_ROOT=%CD%
cd ../../../
b2 -j 4 toolset=intel address-model=64 release > Build\Full\Build_ICC_Release_AMD64.txt

cd ../../
cd Src/Dependencies/Boost
set BOOST_ROOT=%CD%
cd ../../../
bjam -j 4 toolset=clang address-model=64 release > Build\Full\Build_Clang_Release_AMD64.txt

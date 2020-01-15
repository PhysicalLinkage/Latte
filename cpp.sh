#!/use/bin/bash

CXX="g++"
CXXFLAGS="-std=c++17 -Iinclude"
SRC="`basename $1`"
TEST="test.cpp"
APP="build/`basename $1 .hpp`.test"
DEF=`basename $1 .hpp`

if ! [ -d build ]; then
    mkdir build
fi


echo "#define ${DEF^^}_TEST" > $TEST
echo "#include <$SRC>" >> $TEST
$CXX $CXXFLAGS $TEST -o $APP
rm -f $TEST
$APP


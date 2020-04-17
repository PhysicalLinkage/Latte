#!/bin/bash

CXX="g++"
VERTION="-std=c++17"
OPTION="-O1 -Wall"
INCLUDES="-Iinclude"
LIBS="-lcrypto"
TEST_INCLUDES="-Iinclude -Itest/include"

OBJ_DIR="build/object/" 
TEST_OBJ_DIR="test/build/object/"

#CPP_FILES=`find src/*.cpp`
#TEST_CPP_FILES=`find test/src/*.cpp`
#ALL_OBJ_FILES=`find * -name *.o`
MAKEFILE="Makefile"
MAIN_TEST="test/src/main_test.cpp"
TEST_HPP=$2
TEST_APP="test/build/app/test.app"

makefile-all()
{
    echo ".PHONY: all"
    echo "all : \\"
    for cpp_file in ${CPP_FILES[@]}
    do
        obj_file=`basename $cpp_file`
        obj_file=$OBJ_DIR${obj_file/.cpp}.o
        echo "  $obj_file \\"
    done

    for test_cpp_file in ${TEST_CPP_FILES[@]}
    do
        test_obj_file=`basename $test_cpp_file`
        test_obj_file=$TEST_OBJ_DIR${test_obj_file/.cpp}.o
        echo "  $test_obj_file \\"
    done
    echo " "
}

makefile-dependence()
{
    obj_files=()
    for cpp_file in ${CPP_FILES[@]}
    do
        echo -n "$OBJ_DIR"
        $CXX $INCLUDES -MM $cpp_file
        echo -e "\t$CXX -c \$< -o \$@ $VERTION $OPTION $INCLUDES"
    done

    test_obj_files=()
    for test_cpp_file in ${TEST_CPP_FILES[@]}
    do
        echo -n "$TEST_OBJ_DIR"
        test_obj_file=${test_cpp_file/.cpp}.o
        $CXX $TEST_INCLUDES -MM $test_cpp_file
        echo -e "\t$CXX -c \$< -o \$@ $VERTION $OPTION $TEST_INCLUDES"
    done
}


update-makefile()
{

    makefile-all        >   $MAKEFILE
    makefile-dependence >>  $MAKEFILE
}

test-cpp()
{
    base=`basename $TEST_HPP`
    func=${base/.hpp}

    echo ""                         >   $MAIN_TEST
    echo "#include<$base>"          >>  $MAIN_TEST
    echo ""                         >>  $MAIN_TEST
    echo "int main()"               >>  $MAIN_TEST
    echo "{"                        >>  $MAIN_TEST
    echo "    return ${func}();"    >>  $MAIN_TEST
    echo "}"                        >>  $MAIN_TEST
    echo ""                         >>  $MAIN_TEST
    

    $CXX $VERTION $OPTION $TEST_INCLUDES $MAIN_TEST -o $TEST_APP $LIBS
    rm $MAIN_TEST
    $TEST_APP
}

case $1 in
    "update-makefile"   ) update-makefile   ;;
    "test-cpp"          ) test-cpp          ;;
    *                   ) echo "error"      ;;
esac








.PHONY: all
all : \
  build/object/frame.o \
 
build/object/frame.o: src/frame.cpp include/frame.hpp
	g++ -c $< -o $@ -std=c++17 -O1 -Wall -Iinclude

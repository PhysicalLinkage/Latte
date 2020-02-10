.PHONY: all
all : \
  build/object/behavior.o \
  build/object/frame.o \
  build/object/state.o \
  test/build/object/test_state.o \
 
build/object/behavior.o: src/behavior.cpp include/state.hpp include/behavior.hpp
	g++ -c $< -o $@ -std=c++17 -O1 -Wall -Iinclude
build/object/frame.o: src/frame.cpp include/frame.hpp
	g++ -c $< -o $@ -std=c++17 -O1 -Wall -Iinclude
build/object/state.o: src/state.cpp include/state.hpp include/behavior.hpp
	g++ -c $< -o $@ -std=c++17 -O1 -Wall -Iinclude
test/build/object/test_state.o: test/src/test_state.cpp test/include/test_state.hpp \
 include/frame_timer.hpp include/frame.hpp include/state.hpp \
 include/behavior.hpp
	g++ -c $< -o $@ -std=c++17 -O1 -Wall -Iinclude -Itest/include

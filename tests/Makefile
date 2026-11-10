CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O1 -g -pthread
SRC      := $(filter-out ../src/main.cpp,$(wildcard ../src/*.cpp))
TESTS    := $(wildcard test_*.cpp)

run_tests: $(TESTS) $(SRC) $(wildcard *.h) $(wildcard ../src/*.h)
	$(CXX) $(CXXFLAGS) -o $@ $(TESTS) $(SRC)

test: run_tests
	./run_tests

clean:
	rm -f run_tests
.PHONY: test clean

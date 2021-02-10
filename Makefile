# apt-get update
# apt-get install g++
# apt-get install build-essential

CXX = g++
CXXFLAGS = -Wall -Wextra -Werror -g -std=c++17

all: main test bench

main: main.cpp
    $(CXX) $< -o $@ $(CXXFLAGS)

test: test.cpp
    $(CXX) $< -o $@ $(CXXFLAGS)

bench: bench.cpp
    $(CXX) $< -o $@ $(CXXFLAGS)

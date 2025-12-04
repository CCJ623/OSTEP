CC = gcc
CXX = g++
CFLAGS = -g -Wall -O2
CXXFLAGS = -g -Wall -O -std=c++23

# Executables to build (one per corresponding .cpp source)
TARGETS = check_xor check_fletcher benchmark check_crc create_csum check_csum
HEADERS = check_functions.h

.PHONY: all clean

# Default target builds all programs
all: $(TARGETS)

# Generic rule: build an executable from a .cpp source with the same name
# e.g. `check_xor` from `check_xor.cpp`. Rebuild if the header changes.
%: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(TARGETS) *.o
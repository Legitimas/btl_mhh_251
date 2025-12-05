# Compiler
CXX := g++

# Compiler flags
CXXFLAGS := -std=c++17 -Wall -Iinclude -Icudd/include/cudd -ID:/Coding/C++/Utils

# Linker flags (link with prebuilt CUDD library)
LDFLAGS := cudd/build/libcudd.a

# Source files
SRC := $(wildcard src/*.cpp)

# Object files in build/
OBJ := $(SRC:src/%.cpp=build/%.o)

# Target executable
TARGET := main.exe

# Default rule
all: $(TARGET)

# Ensure build directory exists
build:
	mkdir -p build

# Compile .cpp files into build/*.o
build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Link all objects into executable
$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $(TARGET) $(LDFLAGS)

# Clean build artifacts
clean:
	rm -rf build $(TARGET)
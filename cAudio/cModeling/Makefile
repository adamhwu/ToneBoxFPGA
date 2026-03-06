# Compiler
CXX := g++ -g
CXXFLAGS := -std=c++17 -Wall -O0 -Isrc -Iinclude

# Target executable
TARGET := hw_modeler

# Source files (all .cpp in src/)
SRCS := $(wildcard src/*.cpp)
OBJS := $(SRCS:.cpp=.o)

# Default rule
all: $(TARGET)

# Link step
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ 

# Compile step
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean
clean:
	rm -f $(OBJS) $(TARGET)

# Run
test:
	./$(TARGET)
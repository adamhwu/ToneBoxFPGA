# Compiler
CXX := g++

# Compiler flags
CXXFLAGS := -std=c++17 -Wall -Wextra -O2

# Target executable
TARGET := delay

# Source files
SRCS := main.cpp

# Object files (auto-generated from sources)
OBJS := $(SRCS:.cpp=.o)

# Default rule
all: $(TARGET)

# Link step
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^
main.o: main.cpp delay.cpp headers.h
	$(CXX) $(CXXFLAGS) -c $< -o $@
# Compile step
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(TARGET) $(OBJS)

# Convenience target: build and run
test: $(TARGET)
	./$(TARGET)
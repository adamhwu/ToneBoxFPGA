# Compiler
CXX := g++
CXXFLAGS := -std=c++17 -Wall -O2 -Isrc -Iinclude -I/opt/homebrew/Cellar/portaudio/19.7.0/include

# Target executable
TARGET := realtime_fx

# Source files (all .cpp in src/)
SRCS := $(wildcard src/*.cpp)
OBJS := $(SRCS:.cpp=.o)

# PortAudio library
PA_FLAGS := -lportaudio

# Default rule
all: $(TARGET)
	rm -f $(OBJS)

# Link step
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(PA_FLAGS) -L/opt/homebrew/Cellar/portaudio/19.7.0/lib

# Compile step
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean
clean:
	rm -f $(OBJS) $(TARGET)

# Run
test: $(TARGET)
	./$(TARGET)
# Compiler
CXX := g++
CXXFLAGS := -std=c++17 -Wall -O2 -Isrc -Iinclude -I/opt/homebrew/Cellar/portaudio/19.7.0/include -I/opt/homebrew/Cellar/libsndfile/1.2.2_1/include

# Target executable
TARGET := realtime_fx

# Source files (all .cpp in src/)
SRCS := $(wildcard src/*.cpp) $(wildcard src/effects/*.cpp)
OBJS := $(SRCS:.cpp=.o)

# PortAudio library
PA_FLAGS := -lportaudio -lsndfile

# Library paths
LIBRARY_PATH := /opt/homebrew/Cellar/
LINK_FLAGS := -L$(LIBRARY_PATH)portaudio/19.7.0/lib -L$(LIBRARY_PATH)libsndfile/1.2.2_1/lib

# Default rule
all: $(TARGET)
	rm -f $(OBJS)

# Link step
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(PA_FLAGS) $(LINK_FLAGS)

# Compile step
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean
clean:
	rm -f $(OBJS) $(TARGET)

# Run
test:
	./$(TARGET)
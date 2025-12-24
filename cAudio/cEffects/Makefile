CXX = g++
CXXFLAGS = -std=c++17 -Wall -Iinclude

SRCS = src/main.cpp src/delay.cpp src/distortion.cpp src/reverb.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = delay

all: $(TARGET)
	rm -f $(OBJS)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test:
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
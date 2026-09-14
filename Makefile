CXX = g++
CXXFLAGS = -std=c++14 -O3 -Wall -Wextra -Iinclude -Ithird_party
LDFLAGS = -pthread

SRCS = src/image.cpp src/matrix.cpp src/harris.cpp src/panorama.cpp src/main.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = panorama_cli

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

test: $(TARGET)
	./$(TARGET) --test-all

.PHONY: all clean test

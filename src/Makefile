CC       = gcc
CXX      = g++
CFLAGS   = -std=c11 -Wall -Wextra -Wpedantic -O2
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LDLIBS   = -lyaml
TARGET   = vibe-req

# Main binary sources (C and C++)
C_SRCS   = main.c requirement.c yaml_simple.c discovery.c
CPP_SRCS = triplet_store.cpp triplet_store_c.cpp
C_OBJS   = $(C_SRCS:.c=.o)
CPP_OBJS = $(CPP_SRCS:.cpp=.o)
OBJS     = $(C_OBJS) $(CPP_OBJS)

# Test binary
TEST_TARGET = test_triplet_store
TEST_OBJS   = test_triplet_store.o triplet_store.o triplet_store_c.o

.PHONY: all clean test

all: $(TARGET)

# Link with g++ when C++ objects are present (required for the C++ runtime).
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

$(TEST_TARGET): $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

test: $(TEST_TARGET)
	./$(TEST_TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET) $(TEST_OBJS) $(TEST_TARGET)

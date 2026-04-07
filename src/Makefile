CC       = gcc
CXX      = g++
CFLAGS   = -std=c11 -Wall -Wextra -Wpedantic -O2
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LDLIBS   = -lyaml
TARGET   = vibe-req

# Main binary sources (C and C++)
C_SRCS   = main.c requirement.c yaml_simple.c discovery.c config.c
CPP_SRCS = triplet_store.cpp triplet_store_c.cpp
C_OBJS   = $(C_SRCS:.c=.o)
CPP_OBJS = $(CPP_SRCS:.cpp=.o)
OBJS     = $(C_OBJS) $(CPP_OBJS)

# Test binaries
TEST_TRIPLET_TARGET = test_triplet_store
TEST_TRIPLET_OBJS   = test_triplet_store.o triplet_store.o triplet_store_c.o

TEST_CONFIG_TARGET  = test_config
TEST_CONFIG_OBJS    = test_config.o config.o

.PHONY: all clean test

all: $(TARGET)

# Link with g++ when C++ objects are present (required for the C++ runtime).
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

$(TEST_TRIPLET_TARGET): $(TEST_TRIPLET_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST_CONFIG_TARGET): $(TEST_CONFIG_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

test: $(TEST_TRIPLET_TARGET) $(TEST_CONFIG_TARGET)
	./$(TEST_TRIPLET_TARGET)
	./$(TEST_CONFIG_TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET) \
	      $(TEST_TRIPLET_OBJS) $(TEST_TRIPLET_TARGET) \
	      $(TEST_CONFIG_OBJS) $(TEST_CONFIG_TARGET)

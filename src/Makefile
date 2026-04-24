CC       = gcc
CXX      = g++
CFLAGS   = -std=c11 -Wall -Wextra -Wpedantic -O2
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LDLIBS   = -lyaml
TARGET   = vibe-req

# gtest / gmock — override GTEST_PREFIX at the command line or in the
# environment if your installation differs from the default below.
GTEST_PREFIX  ?= /usr/share/miniconda
GTEST_CFLAGS  = -I$(GTEST_PREFIX)/include
GTEST_LDFLAGS = -L$(GTEST_PREFIX)/lib -Wl,-rpath,$(GTEST_PREFIX)/lib \
                -lgtest_main -lgtest -lgmock -pthread

# Main binary sources (C and C++)
C_SRCS   = main.c requirement.c entity.c yaml_simple.c discovery.c config.c report.c new_cmd.c
CPP_SRCS = triplet_store.cpp triplet_store_c.cpp
C_OBJS   = $(C_SRCS:.c=.o)
CPP_OBJS = $(CPP_SRCS:.cpp=.o)
OBJS     = $(C_OBJS) $(CPP_OBJS)

# Test binaries (sources live in tests/)
TEST_TRIPLET_TARGET = tests/test_triplet_store
TEST_TRIPLET_OBJS   = tests/test_triplet_store.o triplet_store.o triplet_store_c.o

TEST_CONFIG_TARGET  = tests/test_config
TEST_CONFIG_OBJS    = tests/test_config.o config.o

TEST_YAML_TARGET    = tests/test_yaml_simple
TEST_YAML_OBJS      = tests/test_yaml_simple.o yaml_simple.o requirement.o \
                      entity.o triplet_store.o triplet_store_c.o

TEST_DISCOVERY_TARGET = tests/test_discovery_ignore
TEST_DISCOVERY_OBJS   = tests/test_discovery_ignore.o discovery.o requirement.o \
                        yaml_simple.o config.o triplet_store.o triplet_store_c.o \
                        entity.o

TEST_ENTITY_TARGET = tests/test_entity
TEST_ENTITY_OBJS   = tests/test_entity.o entity.o yaml_simple.o requirement.o \
                     triplet_store.o triplet_store_c.o

TEST_REPORT_TARGET = tests/test_report
TEST_REPORT_OBJS   = tests/test_report.o report.o entity.o yaml_simple.o \
                     requirement.o triplet_store.o triplet_store_c.o

TEST_NEW_CMD_TARGET = tests/test_new_cmd
TEST_NEW_CMD_OBJS   = tests/test_new_cmd.o new_cmd.o entity.o yaml_simple.o \
                      requirement.o triplet_store.o triplet_store_c.o

.PHONY: all clean test

all: $(TARGET)

# Link with g++ when C++ objects are present (required for the C++ runtime).
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

$(TEST_TRIPLET_TARGET): $(TEST_TRIPLET_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(GTEST_LDFLAGS)

$(TEST_CONFIG_TARGET): $(TEST_CONFIG_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS) $(GTEST_LDFLAGS)

$(TEST_YAML_TARGET): $(TEST_YAML_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS) $(GTEST_LDFLAGS)

$(TEST_DISCOVERY_TARGET): $(TEST_DISCOVERY_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS) $(GTEST_LDFLAGS)

$(TEST_ENTITY_TARGET): $(TEST_ENTITY_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS) $(GTEST_LDFLAGS)

$(TEST_REPORT_TARGET): $(TEST_REPORT_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS) $(GTEST_LDFLAGS)

$(TEST_NEW_CMD_TARGET): $(TEST_NEW_CMD_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS) $(GTEST_LDFLAGS)

test: $(TEST_TRIPLET_TARGET) $(TEST_CONFIG_TARGET) $(TEST_YAML_TARGET) $(TEST_DISCOVERY_TARGET) $(TEST_ENTITY_TARGET) $(TEST_REPORT_TARGET) $(TEST_NEW_CMD_TARGET)
	./$(TEST_TRIPLET_TARGET)
	./$(TEST_CONFIG_TARGET)
	./$(TEST_YAML_TARGET)
	./$(TEST_DISCOVERY_TARGET)
	./$(TEST_ENTITY_TARGET)
	./$(TEST_REPORT_TARGET)
	./$(TEST_NEW_CMD_TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

tests/%.o: tests/%.cpp
	$(CXX) $(CXXFLAGS) $(GTEST_CFLAGS) -I. -MMD -MP -c -o $@ $<

# Include auto-generated dependency files (if they exist)
-include $(OBJS:.o=.d) \
         $(TEST_TRIPLET_OBJS:.o=.d) \
         $(TEST_CONFIG_OBJS:.o=.d) \
         $(TEST_YAML_OBJS:.o=.d) \
         $(TEST_DISCOVERY_OBJS:.o=.d) \
         $(TEST_ENTITY_OBJS:.o=.d) \
         $(TEST_REPORT_OBJS:.o=.d) \
         $(TEST_NEW_CMD_OBJS:.o=.d)

clean:
	rm -f $(OBJS) $(TARGET) \
	      $(TEST_TRIPLET_OBJS) $(TEST_TRIPLET_TARGET) \
	      $(TEST_CONFIG_OBJS) $(TEST_CONFIG_TARGET) \
	      $(TEST_YAML_OBJS) $(TEST_YAML_TARGET) \
	      $(TEST_DISCOVERY_OBJS) $(TEST_DISCOVERY_TARGET) \
	      $(TEST_ENTITY_OBJS) $(TEST_ENTITY_TARGET) \
	      $(TEST_REPORT_OBJS) $(TEST_REPORT_TARGET) \
	      $(TEST_NEW_CMD_OBJS) $(TEST_NEW_CMD_TARGET) \
	      $(OBJS:.o=.d) \
	      $(TEST_TRIPLET_OBJS:.o=.d) \
	      $(TEST_CONFIG_OBJS:.o=.d) \
	      $(TEST_YAML_OBJS:.o=.d) \
	      $(TEST_DISCOVERY_OBJS:.o=.d) \
	      $(TEST_ENTITY_OBJS:.o=.d) \
	      $(TEST_REPORT_OBJS:.o=.d) \
	      $(TEST_NEW_CMD_OBJS:.o=.d)

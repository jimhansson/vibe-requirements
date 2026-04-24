CC       = gcc
CXX      = g++
CFLAGS   = -std=c11 -Wall -Wextra -Wpedantic -O2
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LDLIBS   = -lyaml

BIN_DIR   = ../bin
BUILD_DIR = build

TARGET   = $(BIN_DIR)/vibe-req

# gtest / gmock — override GTEST_PREFIX at the command line or in the
# environment if your installation differs from the default below.
GTEST_PREFIX  ?= /usr/share/miniconda
GTEST_CFLAGS  = -I$(GTEST_PREFIX)/include
GTEST_LDFLAGS = -L$(GTEST_PREFIX)/lib -Wl,-rpath,$(GTEST_PREFIX)/lib \
                -lgtest_main -lgtest -lgmock -pthread

# Main binary sources (C and C++)
C_SRCS   = main.c requirement.c entity.c yaml_simple.c discovery.c config.c report.c new_cmd.c
CPP_SRCS = triplet_store.cpp triplet_store_c.cpp
C_OBJS   = $(addprefix $(BUILD_DIR)/,$(C_SRCS:.c=.o))
CPP_OBJS = $(addprefix $(BUILD_DIR)/,$(CPP_SRCS:.cpp=.o))
OBJS     = $(C_OBJS) $(CPP_OBJS)

# Test binaries (sources live in tests/)
TEST_TRIPLET_TARGET = $(BIN_DIR)/tests/test_triplet_store
TEST_TRIPLET_OBJS   = $(BUILD_DIR)/tests/test_triplet_store.o \
                      $(BUILD_DIR)/triplet_store.o $(BUILD_DIR)/triplet_store_c.o

TEST_CONFIG_TARGET  = $(BIN_DIR)/tests/test_config
TEST_CONFIG_OBJS    = $(BUILD_DIR)/tests/test_config.o $(BUILD_DIR)/config.o

TEST_YAML_TARGET    = $(BIN_DIR)/tests/test_yaml_simple
TEST_YAML_OBJS      = $(BUILD_DIR)/tests/test_yaml_simple.o \
                      $(BUILD_DIR)/yaml_simple.o $(BUILD_DIR)/requirement.o \
                      $(BUILD_DIR)/entity.o $(BUILD_DIR)/triplet_store.o \
                      $(BUILD_DIR)/triplet_store_c.o

TEST_DISCOVERY_TARGET = $(BIN_DIR)/tests/test_discovery_ignore
TEST_DISCOVERY_OBJS   = $(BUILD_DIR)/tests/test_discovery_ignore.o \
                        $(BUILD_DIR)/discovery.o $(BUILD_DIR)/requirement.o \
                        $(BUILD_DIR)/yaml_simple.o $(BUILD_DIR)/config.o \
                        $(BUILD_DIR)/triplet_store.o $(BUILD_DIR)/triplet_store_c.o \
                        $(BUILD_DIR)/entity.o

TEST_ENTITY_TARGET = $(BIN_DIR)/tests/test_entity
TEST_ENTITY_OBJS   = $(BUILD_DIR)/tests/test_entity.o $(BUILD_DIR)/entity.o \
                     $(BUILD_DIR)/yaml_simple.o $(BUILD_DIR)/requirement.o \
                     $(BUILD_DIR)/triplet_store.o $(BUILD_DIR)/triplet_store_c.o

TEST_REPORT_TARGET = $(BIN_DIR)/tests/test_report
TEST_REPORT_OBJS   = $(BUILD_DIR)/tests/test_report.o $(BUILD_DIR)/report.o \
                     $(BUILD_DIR)/entity.o $(BUILD_DIR)/yaml_simple.o \
                     $(BUILD_DIR)/requirement.o $(BUILD_DIR)/triplet_store.o \
                     $(BUILD_DIR)/triplet_store_c.o

TEST_NEW_CMD_TARGET = $(BIN_DIR)/tests/test_new_cmd
TEST_NEW_CMD_OBJS   = $(BUILD_DIR)/tests/test_new_cmd.o $(BUILD_DIR)/new_cmd.o \
                      $(BUILD_DIR)/entity.o $(BUILD_DIR)/yaml_simple.o \
                      $(BUILD_DIR)/requirement.o $(BUILD_DIR)/triplet_store.o \
                      $(BUILD_DIR)/triplet_store_c.o

.PHONY: all clean test

all: $(TARGET)

# Link with g++ when C++ objects are present (required for the C++ runtime).
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

$(TEST_TRIPLET_TARGET): $(TEST_TRIPLET_OBJS) | $(BIN_DIR)/tests
	$(CXX) $(CXXFLAGS) -o $@ $^ $(GTEST_LDFLAGS)

$(TEST_CONFIG_TARGET): $(TEST_CONFIG_OBJS) | $(BIN_DIR)/tests
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS) $(GTEST_LDFLAGS)

$(TEST_YAML_TARGET): $(TEST_YAML_OBJS) | $(BIN_DIR)/tests
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS) $(GTEST_LDFLAGS)

$(TEST_DISCOVERY_TARGET): $(TEST_DISCOVERY_OBJS) | $(BIN_DIR)/tests
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS) $(GTEST_LDFLAGS)

$(TEST_ENTITY_TARGET): $(TEST_ENTITY_OBJS) | $(BIN_DIR)/tests
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS) $(GTEST_LDFLAGS)

$(TEST_REPORT_TARGET): $(TEST_REPORT_OBJS) | $(BIN_DIR)/tests
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS) $(GTEST_LDFLAGS)

$(TEST_NEW_CMD_TARGET): $(TEST_NEW_CMD_OBJS) | $(BIN_DIR)/tests
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS) $(GTEST_LDFLAGS)

test: $(TEST_TRIPLET_TARGET) $(TEST_CONFIG_TARGET) $(TEST_YAML_TARGET) $(TEST_DISCOVERY_TARGET) $(TEST_ENTITY_TARGET) $(TEST_REPORT_TARGET) $(TEST_NEW_CMD_TARGET)
	./$(TEST_TRIPLET_TARGET)
	./$(TEST_CONFIG_TARGET)
	./$(TEST_YAML_TARGET)
	./$(TEST_DISCOVERY_TARGET)
	./$(TEST_ENTITY_TARGET)
	./$(TEST_REPORT_TARGET)
	./$(TEST_NEW_CMD_TARGET)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

$(BUILD_DIR)/tests/%.o: tests/%.cpp | $(BUILD_DIR)/tests
	$(CXX) $(CXXFLAGS) $(GTEST_CFLAGS) -I. -MMD -MP -c -o $@ $<

# Create output directories on demand
$(BIN_DIR) $(BIN_DIR)/tests $(BUILD_DIR) $(BUILD_DIR)/tests:
	mkdir -p $@

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
	rm -rf $(BUILD_DIR) $(BIN_DIR)

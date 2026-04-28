CXX      = g++
CXXFLAGS = -std=c++23 -Wall -Wextra -O2
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

# yaml/ sub-module objects (replaces the old monolithic yaml_simple.o)
YAML_OBJS = $(BUILD_DIR)/yaml/yaml_link_parser.o \
            $(BUILD_DIR)/yaml/entity_parser.o

# Main binary sources (C++)
CPP_SRCS = config.cpp triplet_store.cpp entity.cpp discovery.cpp coverage.cpp list_cmd.cpp new_cmd.cpp report.cpp validate.cpp status.cpp main.cpp cli_args.cpp
CPP_OBJS = $(addprefix $(BUILD_DIR)/,$(CPP_SRCS:.cpp=.o))
OBJS     = $(CPP_OBJS) $(YAML_OBJS)

# Test binaries (sources live in tests/)
TEST_TRIPLET_TARGET = $(BIN_DIR)/tests/test_triplet_store
TEST_TRIPLET_OBJS   = $(BUILD_DIR)/tests/test_triplet_store.o \
                      $(BUILD_DIR)/triplet_store.o

TEST_CONFIG_TARGET  = $(BIN_DIR)/tests/test_config
TEST_CONFIG_OBJS    = $(BUILD_DIR)/tests/test_config.o $(BUILD_DIR)/config.o

TEST_YAML_TARGET    = $(BIN_DIR)/tests/test_yaml_simple
TEST_YAML_OBJS      = $(BUILD_DIR)/tests/test_yaml_simple.o \
                      $(YAML_OBJS) \
                      $(BUILD_DIR)/entity.o $(BUILD_DIR)/triplet_store.o \
                     

TEST_DISCOVERY_TARGET = $(BIN_DIR)/tests/test_discovery_ignore
TEST_DISCOVERY_OBJS   = $(BUILD_DIR)/tests/test_discovery_ignore.o \
                        $(BUILD_DIR)/discovery.o \
                        $(YAML_OBJS) $(BUILD_DIR)/config.o \
                        $(BUILD_DIR)/triplet_store.o \
                        $(BUILD_DIR)/entity.o

TEST_ENTITY_TARGET = $(BIN_DIR)/tests/test_entity
TEST_ENTITY_OBJS   = $(BUILD_DIR)/tests/test_entity.o $(BUILD_DIR)/entity.o \
                     $(YAML_OBJS) \
                     $(BUILD_DIR)/triplet_store.o

TEST_REPORT_TARGET = $(BIN_DIR)/tests/test_report
TEST_REPORT_OBJS   = $(BUILD_DIR)/tests/test_report.o $(BUILD_DIR)/report.o \
                     $(BUILD_DIR)/entity.o $(YAML_OBJS) \
                     $(BUILD_DIR)/triplet_store.o

TEST_NEW_CMD_TARGET = $(BIN_DIR)/tests/test_new_cmd
TEST_NEW_CMD_OBJS   = $(BUILD_DIR)/tests/test_new_cmd.o $(BUILD_DIR)/new_cmd.o \
                      $(BUILD_DIR)/entity.o $(YAML_OBJS) \
                      $(BUILD_DIR)/triplet_store.o

TEST_COVERAGE_TARGET = $(BIN_DIR)/tests/test_coverage
TEST_COVERAGE_OBJS   = $(BUILD_DIR)/tests/test_coverage.o \
                       $(BUILD_DIR)/coverage.o $(BUILD_DIR)/entity.o \
                       $(BUILD_DIR)/triplet_store.o \
                       $(YAML_OBJS)

TEST_STATUS_TARGET = $(BIN_DIR)/tests/test_status
TEST_STATUS_OBJS   = $(BUILD_DIR)/tests/test_status.o \
                     $(BUILD_DIR)/status.o $(BUILD_DIR)/entity.o

TEST_CLI_ARGS_TARGET = $(BIN_DIR)/tests/test_cli_args
TEST_CLI_ARGS_OBJS   = $(BUILD_DIR)/tests/test_cli_args.o \
                       $(BUILD_DIR)/cli_args.o $(BUILD_DIR)/report.o \
                       $(BUILD_DIR)/entity.o $(YAML_OBJS) \
                       $(BUILD_DIR)/triplet_store.o

TEST_MAIN_TARGET = $(BIN_DIR)/tests/test_main
TEST_MAIN_OBJS   = $(BUILD_DIR)/tests/test_main.o \
                   $(BUILD_DIR)/list_cmd.o \
                   $(BUILD_DIR)/entity.o $(YAML_OBJS) \
                   $(BUILD_DIR)/triplet_store.o

TEST_VALIDATE_TARGET = $(BIN_DIR)/tests/test_validate
TEST_VALIDATE_OBJS   = $(BUILD_DIR)/tests/test_validate.o \
                       $(BUILD_DIR)/validate.o \
                       $(BUILD_DIR)/entity.o $(YAML_OBJS) \
                       $(BUILD_DIR)/triplet_store.o

.PHONY: all clean test check-compiler

all: $(TARGET)

# Link with g++ when C++ objects are present (required for the C++ runtime).
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

# check-compiler: compile-time probe that the toolchain supports C++23.
# Run this target to verify the toolchain before building the main project.
check-compiler:
	$(CXX) $(CXXFLAGS) -o /dev/null compat/cxx23_check.cpp
	@echo "Compiler check passed: $(CXX) supports C++23."

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

$(TEST_COVERAGE_TARGET): $(TEST_COVERAGE_OBJS) | $(BIN_DIR)/tests
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS) $(GTEST_LDFLAGS)

$(TEST_STATUS_TARGET): $(TEST_STATUS_OBJS) | $(BIN_DIR)/tests
	$(CXX) $(CXXFLAGS) -o $@ $^ $(GTEST_LDFLAGS)

$(TEST_CLI_ARGS_TARGET): $(TEST_CLI_ARGS_OBJS) | $(BIN_DIR)/tests
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS) $(GTEST_LDFLAGS)

$(TEST_MAIN_TARGET): $(TEST_MAIN_OBJS) | $(BIN_DIR)/tests
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS) $(GTEST_LDFLAGS)

$(TEST_VALIDATE_TARGET): $(TEST_VALIDATE_OBJS) | $(BIN_DIR)/tests
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS) $(GTEST_LDFLAGS)

test: $(TEST_TRIPLET_TARGET) $(TEST_CONFIG_TARGET) $(TEST_YAML_TARGET) $(TEST_DISCOVERY_TARGET) $(TEST_ENTITY_TARGET) $(TEST_REPORT_TARGET) $(TEST_NEW_CMD_TARGET) $(TEST_COVERAGE_TARGET) $(TEST_STATUS_TARGET) $(TEST_CLI_ARGS_TARGET) $(TEST_MAIN_TARGET) $(TEST_VALIDATE_TARGET)
	./$(TEST_TRIPLET_TARGET)
	./$(TEST_CONFIG_TARGET)
	./$(TEST_YAML_TARGET)
	./$(TEST_DISCOVERY_TARGET)
	./$(TEST_ENTITY_TARGET)
	./$(TEST_REPORT_TARGET)
	./$(TEST_NEW_CMD_TARGET)
	./$(TEST_COVERAGE_TARGET)
	./$(TEST_STATUS_TARGET)
	./$(TEST_CLI_ARGS_TARGET)
	./$(TEST_MAIN_TARGET)
	./$(TEST_VALIDATE_TARGET)

$(BUILD_DIR)/yaml/%.o: yaml/%.cpp | $(BUILD_DIR)/yaml
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

$(BUILD_DIR)/tests/%.o: tests/%.cpp | $(BUILD_DIR)/tests
	$(CXX) $(CXXFLAGS) $(GTEST_CFLAGS) -I. -MMD -MP -c -o $@ $<

# Create output directories on demand
$(BIN_DIR) $(BIN_DIR)/tests $(BUILD_DIR) $(BUILD_DIR)/tests $(BUILD_DIR)/yaml:
	mkdir -p $@

# Include auto-generated dependency files (if they exist)
-include $(OBJS:.o=.d) \
         $(TEST_TRIPLET_OBJS:.o=.d) \
         $(TEST_CONFIG_OBJS:.o=.d) \
         $(TEST_YAML_OBJS:.o=.d) \
         $(TEST_DISCOVERY_OBJS:.o=.d) \
         $(TEST_ENTITY_OBJS:.o=.d) \
         $(TEST_REPORT_OBJS:.o=.d) \
         $(TEST_NEW_CMD_OBJS:.o=.d) \
         $(TEST_COVERAGE_OBJS:.o=.d) \
         $(TEST_STATUS_OBJS:.o=.d) \
         $(TEST_CLI_ARGS_OBJS:.o=.d) \
         $(TEST_MAIN_OBJS:.o=.d) \
         $(TEST_VALIDATE_OBJS:.o=.d)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

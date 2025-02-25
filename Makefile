DEBUG := 1
# SANITIZER := 0

# Project configuration
NAME := complain
TARGET_EXEC := $(NAME)
CC := clang
C_STANDARD := c17

# Directory structure
SRC_DIRS := src
BUILD_DIR := build
TEST_DIR := test

# Find source files
SRCS := $(shell find $(SRC_DIRS) -name '*.c')
MAIN_SRC := src/complain.c
# Filter out the main source file from the sources
SRCS_NO_MAIN := $(filter-out $(MAIN_SRC),$(SRCS))

# Generate object files list
OBJS_NO_MAIN := $(SRCS_NO_MAIN:%=$(BUILD_DIR)/%.o)
MAIN_OBJ := $(BUILD_DIR)/$(MAIN_SRC).o
OBJS := $(MAIN_OBJ) $(OBJS_NO_MAIN)
DEPS := $(OBJS:.o=.d)


# Test files
TEST_SRCS := $(shell find $(TEST_DIR) -name '*.c')
TEST_OBJS := $(TEST_SRCS:%=$(BUILD_DIR)/%.o)
TEST_EXEC := $(BUILD_DIR)/test_$(TARGET_EXEC)

# Compiler and linker flags
CFLAGS := -std=$(C_STANDARD) -Wall -Wextra -pedantic -Isrc
CPPFLAGS := -MMD -MP
LDFLAGS := -lcjson -lcriterion

# Debug/Release configuration
ifdef DEBUG
	CFLAGS += -g3 -O0 -ggdb \
			  -Wstrict-prototypes -Wold-style-definition -Wshadow -Wvla \
			  -Wno-unused-parameter -Wno-unused-variable
else
	CFLAGS += -O2
endif

ifdef SANITIZER
	CFLAGS += -fsanitize=address,undefined # Build with sanitisers
	LDFLAGS += -fsanitize=address,undefined
endif


# Build rules
all: $(BUILD_DIR)/$(TARGET_EXEC)

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Test rules
test: all $(TEST_EXEC)
	./$(TEST_EXEC) --verbose -j0

# Link test executable with both test objects and source objects (excluding main)
$(TEST_EXEC): $(TEST_OBJS) $(OBJS_NO_MAIN)
	@mkdir -p $(dir $@)
	$(CC) $(TEST_OBJS) $(OBJS_NO_MAIN) -o $@ $(LDFLAGS)

# Utility rules
clean:
	rm -rf $(BUILD_DIR)

re: clean all

bear:
	bear -- make -B

.PHONY: all test clean re bear

-include $(DEPS)

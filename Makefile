DEBUG := 1
SANITIZER := 1

include .env

# Project configuration
NAME := complain
TARGET_EXEC := $(NAME)
CC := clang
C_STANDARD := c17

# Directory structure
SRC_DIRS := src
BUILD_DIR := build
TEST_DIR := test

# Source files
SRCS := $(shell find $(SRC_DIRS) -name '*.c')
MAIN_SRC := src/complain.c
SRCS_NO_MAIN := $(filter-out $(MAIN_SRC),$(SRCS))

# Object files
OBJS_NO_MAIN := $(SRCS_NO_MAIN:%.c=$(BUILD_DIR)/%.o)
MAIN_OBJ := $(BUILD_DIR)/$(MAIN_SRC:.c=.o)
OBJS := $(MAIN_OBJ) $(OBJS_NO_MAIN)
DEPS := $(OBJS:.o=.d)

# Test files
TEST_SRCS := $(shell find $(TEST_DIR) -name '*.c')
TEST_OBJS := $(TEST_SRCS:%.c=$(BUILD_DIR)/%.o)
TEST_EXEC := $(BUILD_DIR)/test_$(NAME)


# Base compiler and linker flags
CFLAGS := -std=$(C_STANDARD) -Wall -Wextra -pedantic -Isrc
CPPFLAGS := -MMD -MP
LDFLAGS := -lcjson -lcriterion

ifdef DEBUG
	CFLAGS += -g3 -O0 -ggdb -Wstrict-prototypes -Wold-style-definition \
			  -Wshadow -Wvla -Wno-unused-parameter -Wno-unused-variable
else
	CFLAGS += -O2
endif

# Sanitizer configuration
ifdef SANITIZER
	SAN_FLAGS := -fsanitize=address,undefined
	CFLAGS += $(SAN_FLAGS)
	LDFLAGS += $(SAN_FLAGS)
endif

# Criterion options
# -j1 is important for sequential logging...
CRITERION_FLAGS := -j1
CRITERION_VERBOSE := --verbose --filter="test_lsp/*"

.PHONY: all test clean re bear


all: $(BUILD_DIR)/$(NAME)

$(BUILD_DIR)/$(NAME): $(OBJS)
	ASAN_OPTIONS=$(ASAN_FLAGS) $(CC) $^ -o $@ $(LDFLAGS)

# Generic rule for object files
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Test targets
test: $(TEST_EXEC)
	./$< $(CRITERION_FLAGS)

$(TEST_EXEC): $(TEST_OBJS) $(OBJS_NO_MAIN)
	@mkdir -p $(dir $@)
	$(CC) $^ -o $@ $(LDFLAGS)

# Utility targets
clean:
	rm -rf $(BUILD_DIR)

re: clean all

bear:
	@echo "Generating compile_database.json"
	bear -- make -B


-include $(DEPS)

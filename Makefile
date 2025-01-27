##
# complain
#
# @file
# @version 0.1

# include config.mk


NAME := complain

# BASIC DEFINITIONS ###########################################################
## Make flags
MAKEFLAGS += --no-print-directory
debug = "1" # Enable debug build


## Name of our executable
TARGET_EXEC := complain
INSTALL_PATH = TODO


# C compiler
CC := clang

# C standard we are using
C_STANDARD := c17

# Universal flags we want
# -std is for the c standard we want

CFLAGS := -std=$(C_STANDARD) -xc

### Release flags
FLAGS_RELEASE := -Wall -Wextra -O2 -pedantic

### Debug flags


FLAGS_DEBUG := -Wall -Wextra -g3 -O0 -ggdb
FLAGS_DEBUG += -Wextra -Wstrict-prototypes -Wold-style-definition -Wshadow -Wvla
FLAGS_DEBUG += -Wno-unused-parameter -Wno-unused-variable # Disable some warnings
FLAGS_DEBUG += -pedantic # Warn on extensions
FLAGS_DEBUG += -fsanitize=address,undefined # Build with sanitisers

CFLAGS += $(FLAGS_DEBUG)
LDFLAGS := -fsanitize=address,undefined

ifeq ($(debug),1)
$(info Debug build)
else
# CFLAGS += $(FLAGS_RELEASE)
endif

$(info CFLAGS is: $(CFLAGS))




# Config here #################################################################

# Sources and building ########################################################

## Directories
# Source directories
SRC_DIRS := src
# INCLUDE_DIR := src # Lets just keep sourcefiles and header files together
# Build directory
BUILD_DIR := build

# LIB_DIR := lib
# LIBRARIES :=

# TEST_DIR := test
# TEST_LIBS := criterion

# LIB_DIRS := $(addprefix $(LIB_DIR)/, $(LIBRARIES))

# Automatically find .c files in SRC_DIRS and LIB_DIR
# SRCS := $(shell find $(SRC_DIRS) $(LIB_DIRS) -name '*.c')
SRCS := $(shell find $(SRC_DIRS) -name '*.c')
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

# Test source files
# TEST_SRCS := $(shell find $(TEST_DIR) -name '*.c')
# TEST_SRCS := $(TEST_DIR)/test_main.c
# TEST_OBJS := $(TEST_SRCS:%=$(BUILD_DIR)/%.o)

# Automatically find include directories in SRC_DIRS, INCLUDE_DIR, and LIB_DIR
# INC_DIRS := $(INCLUDE_DIR) $(shell find $(SRC_DIRS) $(LIB_DIRS) -type d)
# INC_FLAGS := $(addprefix -I,$(INC_DIRS))

MAIN_SRC := src/main.c
MAIN_OBJ := $(BUILD_DIR)/$(MAIN_SRC).o

OBJS_NO_MAIN := $(filter-out $(MAIN_OBJ), $(OBJS))

### Flag assignment for our C compiler
# These flags produce the dependency files
# -MMD and -MP gen .D files for us which let us handle deps automatically
# CPPFLAGS += -MMD -MP $(addprefix -I,$(INC_DIRS))
CPPFLAGS += -MMD -MP
# LDFLAGS += -L$(LIB_DIR) # Library paths
# Linker flags for tests (include Criterion library)
# TEST_LDFLAGS := $(LDFLAGS) -lcriterion
## Targets
# Basic Command Definitions ###################################################
## rm
RM := rm -f
## mkdir
MKDIR_P := mkdir -p

all: $(BUILD_DIR)/$(TARGET_EXEC)

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
# $(CC) $(OBJS) -o $@

$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
# $(CC) $(CPPFLAGS) $(CFLAGS) $(INC_FLAGS) -c $< -o $@

# Test executable name
# TEST_EXEC := $(BUILD_DIR)/test_$(TARGET_EXEC)

# Test target
.PHONY: test

# test: $(TEST_EXEC)
# 	./$(TEST_EXEC)
#
# $(TEST_EXEC): $(OBJS_NO_MAIN) $(TEST_OBJS)
# 	$(CC) $(OBJS_NO_MAIN) $(TEST_OBJS) -o $@ $(TEST_LDFLAGS)

# install: all
# cp -f "$(BUILD_DIR)/$(TARGET_EXEC)" "$(INSTALL_PATH)"
# @echo "TODO"

# uninstall:
# 	@echo "TODO"

re:
	$(MAKE) fclean
	$(MAKE) all

run: all
	-./$(BUILD_DIR)/$(TARGET_EXEC)

clean:
	$(RM) -r $(BUILD_DIR)

bear:
	bear -- make -B

fclean: clean

.PHONY: re run clean fclean install uninstall all
-include $(DEPS)

# end

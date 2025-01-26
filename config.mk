# config.mk

NAME := complain

debug ?= 1 # Enable debug build


## Name of our executable
TARGET_EXEC := complain
INSTALL_PATH = TODO


# C compiler
CC := clang

# C standard we are using
C_STANDARD := c17

# Universal flags we want
# -std is for the c standard we want

CFLAGS = -std=$(C_STANDARD) -xc

### Release flags
FLAGS_RELEASE = -Wall -Wextra -O2 -pedantic

### Debug flags


FLAGS_DEBUG = -Wall -Wextra -ggdb -g3 -O0
FLAGS_DEBUG += -Wextra -Wstrict-prototypes -Wold-style-definition -Wshadow -Wvla
FLAGS_DEBUG += -Wno-unused-parameter -Wno-unused-variable # Disable some warnings
FLAGS_DEBUG += -pedantic # Warn on extensions
FLAGS_DEBUG += -fsanitize=address,undefined # Build with sanitisers

ifeq ($(debug), 1)
  CFLAGS := $(CFLAGS) $(FLAGS_DEBUG)
else
  CFLAGS := $(CFLAGS) $(FLAGS_RELEASE)
endif

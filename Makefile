###############################################################################
# Project properties
###############################################################################

PROJECT_NAME = c-cfg-builder
PROJECT_VERSION = 0.1.0
PROJECT_TYPE = BIN

###############################################################################
# Project directory structure variables and rules
###############################################################################

# Project directories
SRC_DIR = ./src
INCLUDE_DIR = ./include
LIB_DIR = ./lib
TEST_DIR = ./test

BUILD_DIR = ./build
DEP_DIR = $(BUILD_DIR)/dep
BIN_DIR = $(BUILD_DIR)/bin
OBJ_DIR = $(BUILD_DIR)/obj

# Search directories
vpath %.o $(OBJ_DIR)
vpath %.c $(shell find $(SRC_DIR) -type d -printf "%p ")
vpath %.h $(INCLUDE_DIR) $(shell find $(SRC_DIR) -type d -printf "%p ")

###############################################################################
# Build properties
###############################################################################

# Build type ------------------------------------------------------------------

# Build type: DEBUG, RELEASE
BUILD_TYPE = DEBUG

# Tools -----------------------------------------------------------------------

# Compiler
CC = gcc

# Archiver
AR = ar

# Binutils
OBJCOPY = objcopy
OBJDUMP = objdump
SIZE = size

# Path ------------------------------------------------------------------------

# Include path
USR_INCLUDE_DIR :=
INCLUDE_PATH := $(abspath $(shell find $(INCLUDE_DIR) -type d -printf "%p "))
INCLUDE_PATH += $(abspath $(USR_INCLUDE_DIR))

# Library path
USR_LIB_DIR :=
LIB_PATH := $(abspath $(shell find $(LIB_DIR) -type d -printf "%p "))
LIB_PATH += $(abspath $(USR_LIB_DIR))

# Libraries -------------------------------------------------------------------

LIB := ustring clang

# Defines ----------------------------------------------------------------

USR_DEFINES :=

# Flags -----------------------------------------------------------------------

# Preprocessor flags
CPP_FLAGS := $(addprefix -I ,$(INCLUDE_PATH)) $(addprefix -D ,$(USR_DEFINES))

# Compiler flags
CC_FLAGS := \
	-std=c11 \
	-Wall \
	-Wpedantic

CC_DEBUG_FLAGS := -Og -g
CC_RELEASE_FLAGS := -O2 -D NDEBUG

ifeq ($(BUILD_TYPE), DEBUG)
CC_FLAGS += $(CC_DEBUG_FLAGS)
else ifeq ($(BUILD_TYPE), RELEASE)
CC_FLAGS += $(CC_RELEASE_FLAGS)
else
$(error Invalid parameter BUILD_TYPE. Possible values: DEBUG, RELEASE)
endif

# Linker flags
LD_FLAGS := $(addprefix -L ,$(LIB_PATH)) $(addprefix -l ,$(LIB))

###############################################################################
# Build rules
###############################################################################

# Target rule selector
ifeq ($(PROJECT_TYPE), BIN)
TARGET_RULE = build-bin
else ifeq ($(PROJECT_TYPE), SLIB)
TARGET_RULE = build-static-lib
else
$(error Invalid PROJECT_TYPE. Possible values: BIN, SLIB)
endif

# List of source files
SOURCES := $(notdir $(shell find $(SRC_DIR) -type f -name "*.c" -printf "%p "))

# List of object files
OBJECTS := $(SOURCES:.c=.o)

# Output files
BIN_OUTPUT = $(BIN_DIR)/$(PROJECT_NAME)
OBJ_OUTPUT = $(addprefix $(OBJ_DIR)/,$(OBJECTS))

# Build all
.PHONY: all
all: $(TARGET_RULE)

# Generate dependency
include $(SOURCES:%.c=$(DEP_DIR)/%.d)

# Build dependency files
$(DEP_DIR)/%.d: %.c | $(DEP_DIR)
	@set -e;\
	rm -f $@;\
	$(CC) -E -MM -MF $@.tmp $(CPP_FLAGS) $<;\
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.tmp > $@;\
	rm -f $@.tmp

# Build object file from C-source
%.o: %.c | $(OBJ_DIR)
	@echo; echo "Building: $@"
	$(CC) -c $(CPP_FLAGS) $(CC_FLAGS) -o $(OBJ_DIR)/$@ $<

# Build binary from object files
.PHONY: build-bin
build-bin: $(BIN_OUTPUT)

$(BIN_OUTPUT): $(OBJECTS) | $(BIN_DIR)
	@echo; echo "Linking: $(PROJECT_NAME)"
	$(CC) $(CC_FLAGS) -o $@ $(OBJ_OUTPUT) $(LD_FLAGS)

# Build static library from object files
.PHONY: build-static-lib
build-static-lib: $(OBJECTS)
	@echo; echo "Creating archive: lib$(PROJECT_NAME).a"
	$(AR) crs $(BIN_DIR)/lib$(PROJECT_NAME).a $^

# Rules for creating a build directory tree
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(DEP_DIR): $(BUILD_DIR)
	@mkdir -p $(DEP_DIR)

$(BIN_DIR): $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)

$(OBJ_DIR): $(BUILD_DIR)
	@mkdir -p $(OBJ_DIR)

###############################################################################
# Utility rules
###############################################################################

.PHONY: clean
clean:
	@echo; echo "Cleaning project"
	@rm -f -r $(BUILD_DIR)
	@rm -f -r $(TEST_DIR)/build

###############################################################################
# Test rules
###############################################################################
 
export PROJECT_NAME PROJECT_VERSION

export OBJ_DIR OBJECTS
export CC
export CPP_FLAGS CC_FLAGS LD_FLAGS

# Object file containing entry point
export MAIN_OBJECT := main.o

.PHONY: test
test: $(OBJECTS)
	@echo; echo "Running tests"; echo
	@$(MAKE) -C $(TEST_DIR) --no-print-directory

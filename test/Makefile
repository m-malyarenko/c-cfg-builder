BASE_DIR := ..

TEST_BUILD_DIR := ./build
TEST_DEP_DIR := $(TEST_BUILD_DIR)/dep
TEST_OBJ_DIR := $(TEST_BUILD_DIR)/obj
TEST_BIN_DIR := $(TEST_BUILD_DIR)/bin

vpath %.o $(PROJECT_ROOT_DIR)/$(OBJ_DIR) $(TEST_OBJ_DIR)
vpath %.c $(shell find . -type d -printf "%p ")

# List of source files
TEST_SOURCES := $(notdir $(shell find . -type f -name "*.c" -printf "%p "))

# List of object files
TEST_OBJECTS := $(TEST_SOURCES:.c=.o)
APP_OBJECTS := $(filter-out $(MAIN_OBJECT),$(OBJECTS))

# Output files
BIN_OUTPUT := $(TEST_BIN_DIR)/$(PROJECT_NAME)_test
OBJ_OUTPUT := \
	$(addprefix $(TEST_OBJ_DIR)/,$(TEST_OBJECTS)) \
	$(addprefix $(BASE_DIR)/$(OBJ_DIR)/,$(APP_OBJECTS))

.PHONY: all
all: $(BIN_OUTPUT)
	@$(BIN_OUTPUT) --verbose=1

# Generate dependency
include $(TEST_SOURCES:%.c=$(TEST_DEP_DIR)/%.d)

# Build dependency files
$(TEST_DEP_DIR)/%.d: %.c | $(TEST_DEP_DIR)
	@set -e;\
	rm -f $@;\
	$(CC) -E -MM -MF $@.tmp $(CPP_FLAGS) $<;\
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.tmp > $@;\
	rm -f $@.tmp

# Build object file from C-source
%.o: %.c | $(TEST_OBJ_DIR)
	@$(CC) -c $(CPP_FLAGS) $(CC_FLAGS) -o $(TEST_OBJ_DIR)/$@ $<

# Build binary from object files
$(BIN_OUTPUT): $(TEST_OBJECTS) | $(TEST_BIN_DIR)
	@$(CC) $(CC_FLAGS) -o $@ $(OBJ_OUTPUT) $(LD_FLAGS) -l criterion

# Rules for creating a build directory tree
$(TEST_BUILD_DIR):
	@mkdir -p $(TEST_BUILD_DIR)

$(TEST_DEP_DIR): $(TEST_BUILD_DIR)
	@mkdir -p $(TEST_DEP_DIR)

$(TEST_OBJ_DIR): $(TEST_BUILD_DIR)
	@mkdir -p $(TEST_OBJ_DIR)

$(TEST_BIN_DIR): $(TEST_BUILD_DIR)
	@mkdir -p $(TEST_BIN_DIR)

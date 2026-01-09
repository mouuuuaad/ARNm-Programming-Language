
CC := gcc
AS := as
CFLAGS := -std=c11 -Wall -Wextra -O2 -g -pthread -D_GNU_SOURCE
LDFLAGS := -pthread

INC_DIR := include
SRC_DIR := src
ASM_DIR := asm
BUILD_DIR := build
TEST_DIR := tests

C_SRCS := $(SRC_DIR)/runtime.c $(SRC_DIR)/process.c $(SRC_DIR)/scheduler.c \
          $(SRC_DIR)/mailbox.c $(SRC_DIR)/memory.c

UNAME_M := $(shell uname -m)
ifeq ($(UNAME_M),x86_64)
    ASM_SRCS := $(ASM_DIR)/context_x86_64.S
else ifeq ($(UNAME_M),aarch64)
    ASM_SRCS := $(ASM_DIR)/context_arm64.S
else
    $(error Unsupported architecture: $(UNAME_M))
endif

C_OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SRCS))
ASM_OBJS := $(patsubst $(ASM_DIR)/%.S,$(BUILD_DIR)/%.o,$(ASM_SRCS))
OBJS := $(C_OBJS) $(ASM_OBJS)
LIBRARY := $(BUILD_DIR)/libarnm.a

.PHONY: all clean test dirs

all: dirs $(LIBRARY)

dirs:
	@mkdir -p $(BUILD_DIR)

$(LIBRARY): $(OBJS)
	ar rcs $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -I$(INC_DIR) -c -o $@ $<

$(BUILD_DIR)/%.o: $(ASM_DIR)/%.S
	$(CC) -c -o $@ $<

test: dirs $(LIBRARY) test_basic test_spawn test_mailbox

test_basic: $(TEST_DIR)/test_basic.c $(LIBRARY)
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $(BUILD_DIR)/test_basic $< -L$(BUILD_DIR) -larnm $(LDFLAGS)
	@echo "Running basic test..."
	@$(BUILD_DIR)/test_basic

test_spawn: $(TEST_DIR)/test_spawn.c $(LIBRARY)
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $(BUILD_DIR)/test_spawn $< -L$(BUILD_DIR) -larnm $(LDFLAGS)
	@echo "Running spawn test..."
	@$(BUILD_DIR)/test_spawn

test_mailbox: $(TEST_DIR)/test_mailbox.c $(LIBRARY)
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $(BUILD_DIR)/test_mailbox $< -L$(BUILD_DIR) -larnm $(LDFLAGS)
	@echo "Running mailbox test..."
	@$(BUILD_DIR)/test_mailbox

clean:
	rm -rf $(BUILD_DIR)

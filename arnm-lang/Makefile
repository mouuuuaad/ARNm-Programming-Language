
CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -O2 -g
LDFLAGS :=

SRC_DIR := compiler/src
INC_DIR := compiler/include
TEST_DIR := compiler/tests
BUILD_DIR := build

SRCS := $(SRC_DIR)/lexer.c $(SRC_DIR)/parser.c $(SRC_DIR)/types.c \
        $(SRC_DIR)/symbol.c $(SRC_DIR)/sema.c $(SRC_DIR)/ir.c $(SRC_DIR)/irgen.c \
        $(SRC_DIR)/codegen.c $(SRC_DIR)/main.c

ASM_SRC := asm/x86_64/codegen.c
ASM_OBJ := $(BUILD_DIR)/codegen_x86.o

OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS)) $(ASM_OBJ)

TEST_LEXER_SRCS := $(TEST_DIR)/test_lexer.c $(SRC_DIR)/lexer.c
TEST_PARSER_SRCS := $(TEST_DIR)/test_parser.c $(SRC_DIR)/lexer.c $(SRC_DIR)/parser.c
TEST_SEMA_SRCS := $(TEST_DIR)/test_sema.c $(SRC_DIR)/lexer.c $(SRC_DIR)/parser.c \
                  $(SRC_DIR)/types.c $(SRC_DIR)/symbol.c $(SRC_DIR)/sema.c
TEST_IR_SRCS := $(TEST_DIR)/test_ir.c $(SRC_DIR)/ir.c $(SRC_DIR)/types.c
TEST_IRGEN_SRCS := $(TEST_DIR)/test_irgen.c $(SRC_DIR)/irgen.c $(SRC_DIR)/ir.c \
                   $(SRC_DIR)/sema.c $(SRC_DIR)/symbol.c $(SRC_DIR)/types.c \
                   $(SRC_DIR)/parser.c $(SRC_DIR)/lexer.c
TEST_CODEGEN_SRCS := $(TEST_DIR)/test_codegen.c $(SRC_DIR)/codegen.c $(SRC_DIR)/irgen.c \
                     $(SRC_DIR)/ir.c $(SRC_DIR)/sema.c $(SRC_DIR)/symbol.c \
                     $(SRC_DIR)/types.c $(SRC_DIR)/parser.c $(SRC_DIR)/lexer.c

TARGET := $(BUILD_DIR)/arnmc

.PHONY: all clean test test_lexer test_parser test_sema test_ir test_irgen test_codegen dirs

all: dirs $(TARGET)

dirs:
	@mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -I$(INC_DIR) -c -o $@ $<

$(ASM_OBJ): $(ASM_SRC)
	$(CC) $(CFLAGS) -I$(INC_DIR) -c -o $@ $<

test: test_lexer test_parser test_sema test_ir test_irgen test_codegen
	@echo "All tests passed!"

test_lexer: dirs
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $(BUILD_DIR)/test_lexer $(TEST_LEXER_SRCS)
	@echo "Running lexer tests..."
	@$(BUILD_DIR)/test_lexer

test_parser: dirs
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $(BUILD_DIR)/test_parser $(TEST_PARSER_SRCS)
	@echo "Running parser tests..."
	@$(BUILD_DIR)/test_parser

test_sema: dirs
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $(BUILD_DIR)/test_sema $(TEST_SEMA_SRCS)
	@echo "Running sema tests..."
	@$(BUILD_DIR)/test_sema

test_ir: dirs
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $(BUILD_DIR)/test_ir $(TEST_IR_SRCS)
	@echo "Running IR tests..."
	@$(BUILD_DIR)/test_ir

test_irgen: dirs
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $(BUILD_DIR)/test_irgen $(TEST_IRGEN_SRCS)
	@echo "Running IR Gen tests..."
	@$(BUILD_DIR)/test_irgen

test_codegen: dirs
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $(BUILD_DIR)/test_codegen $(TEST_CODEGEN_SRCS)
	@echo "Running Codegen tests..."
	@$(BUILD_DIR)/test_codegen


# Runtime integration
RUNTIME_DIR := runtime
RUNTIME_LIB := $(RUNTIME_DIR)/build/libarnm.a
CRT0_SRC := $(RUNTIME_DIR)/src/crt0.c
CRT0_OBJ := $(BUILD_DIR)/crt0.o

.PHONY: runtime integration

runtime:
	$(MAKE) -C $(RUNTIME_DIR)

$(CRT0_OBJ): $(CRT0_SRC) | dirs
	$(CC) $(CFLAGS) -I$(RUNTIME_DIR)/include -c -o $@ $<

integration: dirs $(TARGET) runtime $(CRT0_OBJ)
	# 1. Compile ARNm to x86_64 assembly
	./$(TARGET) --emit-asm examples/pingpong.arnm > $(BUILD_DIR)/pingpong.s
	# 2. Assemble to object file
	gcc -c -o $(BUILD_DIR)/pingpong.o $(BUILD_DIR)/pingpong.s
	# 3. Link everything
	gcc -o $(BUILD_DIR)/pingpong $(CRT0_OBJ) $(BUILD_DIR)/pingpong.o -L$(RUNTIME_DIR)/build -larnm -lpthread
	@echo "Build complete: $(BUILD_DIR)/pingpong"

clean:
	rm -rf $(BUILD_DIR)
	$(MAKE) -C $(RUNTIME_DIR) clean

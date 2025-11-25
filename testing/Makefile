# Makefile for Config Parser Fuzzing Project
# CSE 731 - Software Testing

CC = gcc
AFL_CC = AFLplusplus/afl-clang-fast
CFLAGS = -Wall -Wextra -std=c11

SRC_DIR = src
BUILD_DIR = build
TEST_DIR = tests

SOURCE = $(SRC_DIR)/config_parser.c
HEADER = $(SRC_DIR)/config_parser.h

# Binary names
BINARY = config_parser
FUZZ_BINARY = config_parser_fuzz
ASAN_BINARY = config_parser_asan

.PHONY: all
all: help

# Build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Normal build
.PHONY: normal
normal: $(BUILD_DIR)
	$(CC) $(CFLAGS) -O2 $(SOURCE) -o $(BUILD_DIR)/$(BINARY)
	@echo "✅ Built: $(BUILD_DIR)/$(BINARY)"

# Fuzzing build with AFL++ and ASAN
.PHONY: fuzz
fuzz: $(BUILD_DIR)
	@echo "Building with AFL++ + ASAN + UBSAN..."
	AFL_USE_ASAN=1 $(AFL_CC) $(CFLAGS) -g -O1 \
		-fsanitize=address -fsanitize=undefined \
		$(SOURCE) -o $(BUILD_DIR)/$(FUZZ_BINARY)
	@echo "✅ Built: $(BUILD_DIR)/$(FUZZ_BINARY)"

# ASAN build for crash reproduction
.PHONY: asan
asan: $(BUILD_DIR)
	$(CC) $(CFLAGS) -g -O0 \
		-fsanitize=address -fsanitize=undefined \
		$(SOURCE) -o $(BUILD_DIR)/$(ASAN_BINARY)
	@echo "✅ Built: $(BUILD_DIR)/$(ASAN_BINARY)"

# Reproduce a specific crash
.PHONY: reproduce
reproduce: asan
	@if [ -z "$(CRASH)" ]; then \
		echo "Usage: make reproduce CRASH=tests/output/default/crashes/id:000000"; \
		exit 1; \
	fi
	@echo "Reproducing crash: $(CRASH)"
	@echo ""
	./$(BUILD_DIR)/$(ASAN_BINARY) $(CRASH)

# Analyze all crashes
.PHONY: triage
triage: asan
	@echo "Analyzing all crashes..."
	@echo ""
	@if [ -d "$(TEST_DIR)/output/default/crashes" ]; then \
		for crash in $(TEST_DIR)/output/default/crashes/id:*; do \
			if [ -f "$$crash" ]; then \
				echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"; \
				echo "Crash: $$(basename $$crash)"; \
				echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"; \
				timeout 5 ./$(BUILD_DIR)/$(ASAN_BINARY) $$crash 2>&1 | head -20 || true; \
				echo ""; \
			fi; \
		done; \
	else \
		echo "No crashes found."; \
	fi

# Show fuzzing statistics
.PHONY: stats
stats:
	@if [ -f "$(TEST_DIR)/output/default/fuzzer_stats" ]; then \
		echo "Fuzzing Statistics:"; \
		echo ""; \
		cat $(TEST_DIR)/output/default/fuzzer_stats; \
	else \
		echo "No fuzzing results found."; \
	fi

# Clean build files
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	@echo "✅ Cleaned build directory"

# Clean everything
.PHONY: clean-all
clean-all: clean
	rm -rf $(TEST_DIR)/output
	@echo "✅ Cleaned all artifacts"

# Help
.PHONY: help
help:
	@echo "Config Parser Fuzzing Project"
	@echo ""
	@echo "Build commands:"
	@echo "  make fuzz      Build with AFL++ + ASAN"
	@echo "  make asan      Build with ASAN only"
	@echo "  make normal    Build normal binary"
	@echo ""
	@echo "Analysis commands:"
	@echo "  make reproduce CRASH=<file>  Reproduce specific crash"
	@echo "  make triage                  Analyze all crashes"
	@echo "  make stats                   Show fuzzing statistics"
	@echo ""
	@echo "Cleanup:"
	@echo "  make clean      Remove build files"
	@echo "  make clean-all  Remove everything"
	@echo ""
	@echo "Example workflow:"
	@echo "  make fuzz"
	@echo "  ./run_fuzzer.sh 600"
	@echo "  make stats"
	@echo "  make reproduce CRASH=tests/output/default/crashes/id:000000"

.DEFAULT_GOAL := help

# Placeholder build system for the wiper control system.
#
# Toolchain choices here (host gcc, C99) are demo defaults (AGENTS.md
# Section 14) pending final MCU/cross-compiler selection. Every target
# checks for the sources it needs and skips cleanly with a message when
# they don't exist yet, so CI stays green without claiming unverified
# build or test success (AGENTS.md Section 14/17).

CC        ?= gcc
CFLAGS    ?= -std=c99 -Wall -Wextra -Werror -O2
INCLUDES  := -Iinclude
SRC_DIR   := src
TEST_DIR  := tests/unit
BUILD_DIR := build

# RH850 cross-compiler is unselected (AGENTS.md Section 7); override
# CROSS_CC/CROSS_CFLAGS once the toolchain is pinned.
CROSS_CC     ?= rh850-gcc
CROSS_CFLAGS ?= $(CFLAGS)

.PHONY: all build test lint hex clean

all: build test

build:
	@srcs="$$(find $(SRC_DIR) -name '*.c' 2>/dev/null)"; \
	if [ -z "$$srcs" ]; then \
		echo "[build] No sources under $(SRC_DIR)/ yet; nothing to build."; \
	else \
		echo "[build] Compiling: $$srcs"; \
		mkdir -p $(BUILD_DIR); \
		$(CC) $(CFLAGS) $(INCLUDES) -c $$srcs -o $(BUILD_DIR)/app.o; \
	fi

test:
	@tests="$$(find $(TEST_DIR) -name '*.c' 2>/dev/null)"; \
	srcs="$$(find $(SRC_DIR) -name '*.c' 2>/dev/null)"; \
	if [ -z "$$tests" ]; then \
		echo "[test] No unit tests under $(TEST_DIR)/ yet; nothing to run."; \
	else \
		echo "[test] Building and running: $$tests $$srcs"; \
		mkdir -p $(BUILD_DIR); \
		$(CC) $(CFLAGS) $(INCLUDES) $$tests $$srcs -o $(BUILD_DIR)/unit_tests; \
		$(BUILD_DIR)/unit_tests; \
	fi

lint:
	@if command -v cppcheck >/dev/null 2>&1 && [ -d "$(SRC_DIR)" ] && [ -n "$$(find $(SRC_DIR) -name '*.c' 2>/dev/null)" ]; then \
		echo "[lint] Running cppcheck (MISRA add-on)..."; \
		cppcheck --std=c99 --enable=warning,style,performance,portability \
			--addon=misra --error-exitcode=1 --inline-suppr $(INCLUDES) $(SRC_DIR); \
	else \
		echo "[lint] Skipping cppcheck: no sources under $(SRC_DIR)/ or cppcheck unavailable."; \
	fi
	@if command -v clang-tidy >/dev/null 2>&1 && [ -d "$(SRC_DIR)" ] && [ -n "$$(find $(SRC_DIR) -name '*.c' 2>/dev/null)" ]; then \
		echo "[lint] Running clang-tidy..."; \
		clang-tidy $$(find $(SRC_DIR) -name '*.c') -- $(CFLAGS) $(INCLUDES); \
	else \
		echo "[lint] Skipping clang-tidy: no sources under $(SRC_DIR)/ or clang-tidy unavailable."; \
	fi

hex:
	@srcs="$$(find $(SRC_DIR) -name '*.c' 2>/dev/null)"; \
	if command -v $(CROSS_CC) >/dev/null 2>&1 && [ -n "$$srcs" ]; then \
		echo "[hex] Cross toolchain $(CROSS_CC) found; building hex image..."; \
		mkdir -p $(BUILD_DIR); \
		$(CROSS_CC) $(CROSS_CFLAGS) $(INCLUDES) $$srcs -o $(BUILD_DIR)/app.elf; \
		objcopy -O ihex $(BUILD_DIR)/app.elf $(BUILD_DIR)/app.hex; \
	else \
		echo "[hex] Skipping: RH850 cross-toolchain ('$(CROSS_CC)') not available and/or MCU/toolchain not yet selected (see AGENTS.md Section 14)."; \
	fi

clean:
	rm -rf $(BUILD_DIR)

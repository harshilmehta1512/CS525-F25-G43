# Makefile — build both Storage Manager test runners (no test_helper.c needed)
# Toolchain
CC      := gcc
CFLAGS  := -Wall -Wextra -std=c11 -O2

# Headers (for dependency tracking; no test_helper.c exists)
HDRS    := dberror.h storage_mgr.h test_helper.h

# Common sources (no main functions here)
COMMON_SRCS := dberror.c storage_mgr.c

# Runners (each provides its own main and #include's test_assign1_1.c internally)
RUNNER_ALL   := integrated_tester.c
RUNNER_MAIN  := Main_testing_file.c

# Binaries
INTEGRATED_TESTER_BIN   := integrated_tester
MAIN_TESTING_FILE_BIN := Main_testing_file

# Default: build both
.PHONY: all
all: $(INTEGRATED_TESTER_BIN) $(MAIN_TESTING_FILE_BIN)

# Link rules
$(INTEGRATED_TESTER_BIN): $(COMMON_SRCS) $(RUNNER_ALL) $(HDRS)
	$(CC) $(CFLAGS) -o $@ $(COMMON_SRCS) $(RUNNER_ALL)
	chmod +x $@

$(MAIN_TESTING_FILE_BIN): $(COMMON_SRCS) $(RUNNER_MAIN) $(HDRS)
	$(CC) $(CFLAGS) -o $@ $(COMMON_SRCS) $(RUNNER_MAIN)
	chmod +x $@

# Convenience run targets
.PHONY: run run-all run-main
run: $(INTEGRATED_TESTER_BIN) $(MAIN_TESTING_FILE_BIN)
	@echo "=== Running integrated_tests ===" >  integrated_tester_output.txt
	@./$(INTEGRATED_TESTER_BIN)        >> integrated_tester_output.txt
	@echo "Results stored in integrated_tester_output.txt"
	@echo "\n=== Running main_testing_file ===" >> main_testing_file_output.txt
	@./$(MAIN_TESTING_FILE_BIN)      >> main_testing_file_output.txt
	@echo "Results stored in main_testing_file_output.txt"

run-all: $(INTEGRATED_TESTER_BIN)
	./$(INTEGRATED_TESTER_BIN)

run-main: $(MAIN_TESTING_FILE_BIN)
	./$(MAIN_TESTING_FILE_BIN)

# Housekeeping
.PHONY: clean
clean:
	rm -f $(INTEGRATED_TESTER_BIN) $(MAIN_TESTING_FILE_BIN) *.o integrated_tester_output.txt main_testing_file_output.txt

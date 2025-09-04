



# Convenience run targets
.PHONY: run run-all run-main
run: $(ALL_TESTS_BIN) $(MAIN_TESTER_BIN)
	@echo "=== Running all_tests ===" >  output.txt
	@./$(ALL_TESTS_BIN)        >> all_tests_output.txt
	@echo "Results stored in all_tester_output.txt"
	@echo "\n=== Running main_tester ===" >> output.txt
	@./$(MAIN_TESTER_BIN)      >> main_tester_output.txt
	@echo "Results stored in main_tester_output.txt"

run-all: $(ALL_TESTS_BIN)
	./$(ALL_TESTS_BIN)

run-main: $(MAIN_TESTER_BIN)
	./$(MAIN_TESTER_BIN)

# Housekeeping
.PHONY: clean
clean:
	rm -f $(ALL_TESTS_BIN) $(MAIN_TESTER_BIN) *.o output.txt

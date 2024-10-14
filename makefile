CXX = g++
CXXFLAGS = -std=c++17

DEBUGFLAGS = -DDEBUG

BIN_DIR=bin
SRC_DIR=src

TRACE_DIR=trace
STD_TRACE_DIR=$(TRACE_DIR)/STD_Format
TEST_TRACE_DIR=$(TRACE_DIR)/test
HUMANREADABLE_TRACE_DIR=$(TRACE_DIR)/human_readable_trace
FORMATTED_TRACE_DIR=$(TRACE_DIR)/binary_trace

TARGET = $(BIN_DIR)/verify_sc
TRACE_GENERATOR = $(BIN_DIR)/trace_generator

SRC = $(SRC_DIR)/main.cpp
DEPS = $(wildcard $(SRC_DIR)/common/*.cpp) $(wildcard $(SRC_DIR)/consistency_checker/.cpp) $(SRC_DIR)/parsing/parser.cpp

TRACE_GENERATOR_SRC = $(SRC_DIR)/trace_generator.cpp
TRACE_GENERATOR_DEPS = $(SRC_DIR/common/event.cpp) $(SRC_DIR/parsing/trace_conversion.cpp)

STD_CONVERTER=scripts/convert.py
TEST_CONVERTER=scripts/convert_test.py

readable_traces = $(shell ls $(HUMANREADABLE_TRACE_DIR)/*.txt)
INPUT = trace/binary_trace/input1.txt
SIZE = 100

CSV_GENERATOR = scripts/benchmark_to_csv.py
LOG_DIR = logs/output
CSV_DIR = logs/csv

PREP_CSV_GENERATOR = scripts/prep_csv.py
PREP_CSV_DIR = logs/combined

all: $(TARGET)

run: clean $(TARGET)
	@echo "Running with input file: $(INPUT)"
	@echo ""
	@./$(TARGET) $(INPUT) -s $(SIZE) -v

debug: clean $(SRC) $(DEPS)
	mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(DEBUGFLAGS) -o $(TARGET) $(SRC)
	./$(TARGET) $(INPUT) -s $(SIZE) -v

$(TARGET): $(SRC) $(DEPS)
	mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

$(TRACE_GENERATOR): $(TRACE_GENERATOR_SRC) $(TRACE_GENERATOR_DEPS)
	mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $(TRACE_GENERATOR) $(TRACE_GENERATOR_SRC)

gen_from_std_trace:
	mkdir -p $(HUMANREADABLE_TRACE_DIR)
	python3 $(STD_CONVERTER) $(STD_TRACE_DIR) $(HUMANREADABLE_TRACE_DIR)

gen_single_trace: $(TRACE_GENERATOR)
	mkdir -p $(FORMATTED_TRACE_DIR)
	$(TRACE_GENERATOR) $(args) $(FORMATTED_TRACE_DIR)

gen_from_test_trace:
	mkdir -p $(HUMANREADABLE_TRACE_DIR)
	python3 $(TEST_CONVERTER) $(TEST_TRACE_DIR) $(HUMANREADABLE_TRACE_DIR)

gen_traces: $(TRACE_GENERATOR)
	mkdir -p $(FORMATTED_TRACE_DIR)
	@for file in $(readable_traces); do \
		echo "Generating trace for $$file..."; \
		$(TRACE_GENERATOR) $$file $(FORMATTED_TRACE_DIR);\
	done

.PHONY = gen_csv prep_csv
gen_csv:
	python3 $(CSV_GENERATOR) $(LOG_DIR) $(CSV_DIR)

prep_csv:
	python3 $(PREP_CSV_GENERATOR) $(CSV_DIR) $(PREP_CSV_DIR)

# Clean target to remove the compiled files
clean:
	rm -f $(TARGET) $(TRACE_GENERATOR) $(TARGET_STRICT)

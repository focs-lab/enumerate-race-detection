CXX = g++
CXXFLAGS = -std=c++20

DEBUGFLAGS = -DDEBUG

BIN_DIR=bin
SRC_DIR=src

TRACE_DIR=trace
WITNESS_DIR=witness

TARGET = $(BIN_DIR)/verify_sc

SRC = $(wildcard $(SRC_DIR)/*.cpp)
DEPS = $(wildcard $(SRC_DIR)/*.hpp)

INPUT = input.txt
NUM_THREADS = 8

all: $(TARGET)

run: clean $(TARGET)
	@echo "Running with input file: $(INPUT)"
	@echo ""
	@./$(TARGET) $(INPUT) -v -o $(WITNESS_DIR) -p $(NUM_THREADS)

debug: clean $(SRC) $(DEPS)
	mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(DEBUGFLAGS) -o $(TARGET) $(SRC)
	./$(TARGET) $(INPUT) -v -o $(WITNESS_DIR) -p $(NUM_THREADS)

$(TARGET): $(SRC) $(DEPS)
	mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

# Clean target to remove the compiled files
clean:
	rm -f $(TARGET)
	rm -rf $(WTINESS_DIR)

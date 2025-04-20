# Enumerate Race Detection

## Prerequisites 
- gcc (7.1 and above)
- make (4.0 and above)

## Building and Running 

### Building
To build: 
```sh
make all
```

### Command Line flags
The executable `verify_sc` takes in the following flags:

- `-v`, `--verbose`
  - Prints more verbose output to stdout
- `-w`, `--witness`
  - Enables generation of witness for data races predicted 
- `-o <OUTPUT_DIR>`, `--outputDir <OUTPUT_DIR>`
  - <OUTPUT_DIR> for generated witness 
  - Requires `-w` or `--witness` flag for witnesses to be generated
- `-p <NUM_THREADS>`, `--parallel <NUM_THREADS>`
  - Execute <NUM_THREADS> in parallel
  
All flags are optional.

### Running
To run: 

```sh
./bin/verify_sc <INPUT_TRACE> -v -w -o <WITNESS_DIR> -p <NUM_THREADS>
```

For convenience, some additional makefile targets are provided:
```sh
make run INPUT=<INPUT_FILE> NUM_THREADS=<NUM_THREADS>
```

This combines building and running together. Witness generation, however, is disabled for speed. Modify the makefile as needed.


## Trace Format
~enumerate_race_detection~ support the following events: 
- Read/Write
- Acquire/Release
- Begin/End
- Fork/Join

Each event are represented using 64 bits: 4 bits event identifier, 8 bits thread identifier, 20 bits variable dentifer, 32 bits variable value. 

Input traces are assumed to be binary files where each line consists of a 64 bit representing an event.

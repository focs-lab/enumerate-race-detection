# Enumerate Race Detection

## Prerequisites 
- gcc (7.1 and above)
- make (4.0 and above)

## Building and Running 

To build: 
```sh
make all
```

To run: 
```sh
make run INPUT=<INPUT_FILE> 
```

To run with debug info enabled:

```sh
make debug INPUT=<INPUT_FILE>
```

## Trace Generation

The chosen format for an event is an `uint64_t` as follows:
  - 4 bits event identifier
  - 8 bits thread identifier 
  - 20 bits variable
  
This project reads and parses traces from a binary file where each event is represented using the above format. 

For convenience, scripts are provided to convert human-readable as well as STD format to the binary format expected. Use one of `make gen_from_std_trace` to convert to human-readable format and `make gen_traces` to convert from human-readable format to the required binary file. 

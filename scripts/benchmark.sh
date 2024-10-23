#!/bin/bash
INPUT_DIR="trace/binary_trace"
TIME_LIMIT=1200
FILENAME=$1
ITERATIONS=$2

for ((i=10;i<=ITERATIONS;i+=10))
do
    timeout $TIME_LIMIT make benchmark INPUT=$INPUT_DIR/$FILENAME.txt SIZE=$i > "logs/output/${FILENAME}_${i}.log"
    EXIT_CODE=$?

    if [ $EXIT_CODE -eq 124 ]; then
        echo "The program timed out for size: $i"
        break
    elif [ $EXIT_CODE -ne 0 ]; then
        echo "The program encountered an error (exit code: $EXIT_CODE) for size: $i"
    fi

    echo "----------------------------------------------------"
done

echo "Execution completed."
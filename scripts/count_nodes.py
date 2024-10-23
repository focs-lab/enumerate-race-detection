import csv
import sys

def sum_second_column(csv_file):
    total = 0
    with open(csv_file, 'r') as file:
        reader = csv.reader(file)
        next(reader)  # Skip the header row if there's one
        for row in reader:
            total += float(row[1])  # Convert the second column to float and sum it
    return total


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 count_nodes.py <FILE>")
        sys.exit(1)

    input_file = sys.argv[1]

    result = sum_second_column(input_file)
    print(f"Total nodes: {result}")
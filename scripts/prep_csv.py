#!/usr/bin/env python3

import os
import csv
import sys


def prep_log_file(source_file, target_file, window_size):
    file_exists = os.path.isfile(target_file)

    with open(target_file, "a") as combined_file:
        writer = csv.writer(combined_file)

        # Open the source file to read its content
        with open(source_file, "r") as source:
            reader = csv.reader(source)
            header = next(reader)  # Read the header row

            if not file_exists:
                header.append("Window Size")
                writer.writerow(header)

            for row in reader:
                row.append(window_size)
                writer.writerow(row)

    return True


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python convert.py <input_dir> <output_dir>")
        sys.exit(1)

    input_dir = sys.argv[1]
    output_dir = sys.argv[2]

    for filename in os.listdir(input_dir):
        if not filename.endswith(".csv"):
            continue
        input_file = os.path.join(input_dir, filename)
        output_file = os.path.join(output_dir, filename.split("_")[0] + ".csv")
        window_size = filename.split(".")[0].split("_")[1]

        if prep_log_file(input_file, output_file, window_size):
            print(f"Converted {filename}")
        else:
            print(f"Error converting {filename}")
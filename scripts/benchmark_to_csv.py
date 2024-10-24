#!/usr/bin/env python3

import os
import sys


headers = [
    "Window,Num threads,Num Variables,Num locks,Num unique good writes,Estimate Cost,Estimate Time Taken,Num mmaps,Max stack size,Max seen size,Node explored,Node seen before,Time taken (sec),Window result"
]
window_line_cnt = len(headers[0].split(","))


def extract_window_num(line):
    vals = line.split(" ")
    return vals[1]


def extract_metric(line):
    vals = line.split(": ")
    return vals[1]


def process_window(log_lines, idx):
    data = []
    data.append(extract_window_num(log_lines[idx]).rstrip())
    for i in range(idx + 1, idx + window_line_cnt):
        data.append(extract_metric(log_lines[i]).rstrip())

    return ",".join(data)


def convert_log(log_lines):
    rows = list(headers)
    i = 6
    while i < len(log_lines):
        # print(i)
        if log_lines[i].startswith("Window "):
            rows.append(process_window(log_lines, i))
            i += window_line_cnt
        else:
            i += 1

    return "\n".join(rows)


def convert_log_file(input_file, output_file):
    with open(input_file, "r") as f:
        log_lines = f.readlines()

    try:
        converted_log = convert_log(log_lines)
    except Exception as e:
        return False

    with open(output_file, "w") as f:
        f.write(converted_log)

    return True


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python convert.py <input_dir> <output_dir>")
        sys.exit(1)

    input_dir = sys.argv[1]
    output_dir = sys.argv[2]

    for filename in os.listdir(input_dir):
        if not filename.endswith(".log"):
            continue
        input_file = os.path.join(input_dir, filename)
        output_file = os.path.join(output_dir, filename.split(".")[0] + ".csv")

        if convert_log_file(input_file, output_file):
            print(f"Converted {filename}")
        else:
            print(f"Error converting {filename}")

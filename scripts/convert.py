import random
import os
import sys

"""
T2|fork(T1)|0
T1|join(T21)|358
T20|r(V16e92358.199)|374
T20|w(V16e92358.199)|374 or T20|w(V45c470d5[0])|362
T20|acq(L2a45c47085)|361
T20|rel(L2a45c47085)|361
"""


def extract_thread_id(thread):
    return int(thread[1:])


def map_action(action_part):
    value = action_part.split("(")[1][:-1]
    action = ""
    if action_part.startswith("rel"):
        action = "Rel"
    elif action_part.startswith("w"):
        action = "Write"
    elif action_part.startswith("fork"):
        action = "Fork"
    elif action_part.startswith("join"):
        action = "Join"
    elif action_part.startswith("acq"):
        action = "Acq"
    elif action_part.startswith("r"):
        action = "Read"
    else:
        raise ValueError(f"Unknown action: {action_part}")

    return action, value


locks = {}
lock_identifier = 0


def get_lock_id(lock):
    if lock not in locks:
        global lock_identifier
        locks[lock] = lock_identifier
        lock_identifier += 1
    return locks[lock]


vars = {}
var_identifier = 0


def get_var_id(var):
    if var not in vars:
        global var_identifier
        vars[var] = var_identifier
        var_identifier += 1
    return vars[var]


var_values = {}


def get_var_value(var, action):
    if var not in var_values:
        var_values[var] = random.randint(0, 100)
    if action == "Write":
        var_values[var] = random.randint(0, 100)

    return var_values[var]


def action_string(action, value):
    res = ""

    if action == "Read":
        res = f"X_{get_var_id(value)} {get_var_value(value, action)}"
    elif action == "Write":
        res = f"X_{get_var_id(value)} {get_var_value(value, action)}"
    elif action == "Fork":
        res = f"{value[1:]} 0"
    elif action == "Join":
        res = f"{value[1:]} 0"
    elif action == "Acq":
        res = f"{get_lock_id(value)} 0"
    elif action == "Rel":
        res = f"{get_lock_id(value)} 0"

    return res


def parse_trace_line(line):
    parts = line.split("|")
    thread = parts[0]
    action_part = parts[1]

    thread_id = extract_thread_id(thread)
    action, value = map_action(action_part)
    action_str = action_string(action, value)

    res = f"{action} {thread_id} {action_str}"

    if action == "Fork":
        res = res + "\n" + f"Begin {value[1:]} 0 0"
    elif action == "Join":
        res = f"End {value[1:]} 0 0" + "\n" + res

    return res


def convert_trace(trace_lines):
    output_lines = []
    for line in trace_lines:
        try:
            new_line = parse_trace_line(line.strip())
            output_lines.append(new_line)
        except Exception as e:
            print(f"Error parsing line: {line}", end="")
            raise e

    return "\n".join(output_lines)


def convert_trace_file(input_file, output_file):
    with open(input_file, "r") as f:
        trace_lines = f.readlines()

    try:
        converted_trace = convert_trace(trace_lines)
    except Exception as e:
        return False

    with open(output_file, "w") as f:
        f.write(converted_trace)

    return True


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python convert.py <input_dir> <output_dir>")
        sys.exit(1)

    input_dir = sys.argv[1]
    output_dir = sys.argv[2]

    for filename in os.listdir(input_dir):
        if not filename.endswith(".std"):
            continue
        input_file = os.path.join(input_dir, filename)
        output_file = os.path.join(output_dir, filename.split(".")[0] + ".txt")

        if convert_trace_file(input_file, output_file):
            print(f"Converted {filename}")
        else:
            print(f"Error converting {filename}")

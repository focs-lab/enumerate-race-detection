import struct
import sys


def read_binary_file(file_path):
    def get_event_type(raw_event):
        typ = (raw_event >> 60) & 0xF
        match typ:
            case 0:
                return "read"
            case 1:
                return "write"
            case 2:
                return "acq"
            case 3:
                return "rel"

        print(f"Unknown event - {typ}")
        return "unknown"

    def get_thread_id(raw_event):
        return (raw_event >> 52) & 0xFF

    def get_var_id(raw_event):
        return (raw_event >> 32) & 0xFFFFF

    def get_var_value(raw_event):
        return raw_event & 0xFFFFFFFF

    with open(file_path, "rb") as f:
        while True:
            # Read 8 bytes (size of uint64_t) from the file
            data = f.read(8)

            if not data:
                break

            # Unpack the bytes into a uint64_t (8-byte unsigned integer)
            e = struct.unpack("Q", data)[0]  # 'Q' is for unsigned long long (64-bit)

            # Print or process the uint64_t value
            print(f"Raw event (uint64_t): {e}")
            print(
                f"{get_event_type(e)} {get_thread_id(e)} {get_var_id(e)} {get_var_value(e)}"
            )
            print()


if len(sys.argv) < 2:
    print("Usage: <input_file>")

file_path = sys.argv[1]
read_binary_file(file_path)

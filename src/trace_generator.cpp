#include <iostream>
#include <string>

#include "parsing/trace_conversion.cpp"

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cerr << "Usage: <input_file> <output_dir>\n";
    return 0;
  }

  std::string inputFilePath = argv[1];
  std::string outputDir = argv[2];

  bool res = convertToBinaryFile(inputFilePath, outputDir);

  if (!res)
    return 1;

  std::cout << "Generated formatted logs" << std::endl;
  return 0;
}

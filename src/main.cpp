#include "consistency_checker/verify_sc.cpp"
#include "utils/config.cpp"
#include <chrono>
#include <iostream>

auto main(int argc, char *argv[]) -> int {
  if (argc < 2) {
    std::cout << "Usage: <input_file>" << std::endl;
    return 0;
  }

  Config c = parseConfig(argc, argv);

  std::cout << "Window size: " << c.windowSize << std::endl << std::endl;

  auto start = std::chrono::high_resolution_clock::now();
  bool isConsistent = windowing(c.inputFile.value(), c.windowSize, c.verbose);

#ifdef DEBUG
  std::cout << std::endl;
#endif

  std::cout << std::boolalpha << "Is sequentially consistent: " << isConsistent
            << std::endl;
  auto end = std::chrono::high_resolution_clock::now();
  std::cout << "Verify_SC took: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end -
                                                                     start)
                       .count() /
                   1000.0
            << "s" << std::endl;
  return 0;
}

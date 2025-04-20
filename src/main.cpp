#include "config.hpp"
#include "parser.hpp"
#include "predictor.hpp"
#include <iostream>

auto main(int argc, char *argv[]) -> int {
  if (argc < 2) {
    std::cout << "Usage: <input_file>" << std::endl;
    return 0;
  }

  try {
    Option opts = parseOptions(argc, argv);
    auto start = std::chrono::high_resolution_clock::now();

    ParseResult pr = parse(opts.inputFile.value());
    Predictor pred{pr, opts};
    pred.predict();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::high_resolution_clock::now() - start) /
                    1000.0;
    std::cout << "Total time taken (sec): " << duration.count() << std::endl;

    pred.reportRaces(opts);
  } catch (const std::exception &e) {
    std::cout << e.what() << std::endl;
  }

  return 0;
}

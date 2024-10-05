#include <functional>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>

constexpr size_t DEFAULT_WINDOW = 2;

struct Config {
  bool verbose = false;
  size_t windowSize = DEFAULT_WINDOW;
  std::optional<std::string> inputFile;
};

typedef std::function<void(Config &)> NoArgHandle;
const std::unordered_map<std::string, NoArgHandle> NoArgs{
    {"--verbose", [](Config &s) { s.verbose = true; }},
    {"-v", [](Config &s) { s.verbose = true; }},
};

typedef std::function<void(Config &, const std::string &)> OneArgHandle;
const std::unordered_map<std::string, OneArgHandle> OneArgs{
    {"-s", [](Config &s,
              const std::string &size) { s.windowSize = std::stoi(size); }},
    {"--size", [](Config &s,
                  const std::string &size) { s.windowSize = std::stoi(size); }},
};

Config parseConfig(int argc, char *argv[]) {
  Config conf;
  for (int i = 1; i < argc; i++) {
    std::string opt{argv[i]};

    // Is this a NoArg?
    if (auto j = NoArgs.find(opt); j != NoArgs.end()) {
      j->second(conf);
    } else if (auto k = OneArgs.find(opt); k != OneArgs.end())
      // Verify there is an arg provided
      if (++i < argc)
        k->second(conf, {argv[i]});
      else
        throw std::runtime_error{"missing param after " + opt};
    else if (!conf.inputFile)
      // No, use this as the input file
      conf.inputFile = argv[i];
    else
      std::cerr << "unrecognized command-line option " << opt << std::endl;
  }

  return conf;
}

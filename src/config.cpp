#include "config.hpp"

Option parseOptions(int argc, char *argv[]) {
  Option opts;
  for (int i = 1; i < argc; i++) {
    std::string opt{argv[i]};

    // Is this a NoArg?
    if (auto j = NoArgs.find(opt); j != NoArgs.end()) {
      j->second(opts);
    } else if (auto k = OneArgs.find(opt); k != OneArgs.end())
      // Verify there is an arg provided
      if (++i < argc)
        k->second(opts, {argv[i]});
      else
        throw std::runtime_error{"missing param after " + opt};
    else if (!opts.inputFile)
      // No, use this as the input file
      opts.inputFile = argv[i];
    else
      std::cerr << "unrecognized command-line option " << opt << std::endl;
  }

  return opts;
}

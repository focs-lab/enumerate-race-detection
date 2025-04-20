#pragma once

#include <functional>
#include <iostream>
#include <optional>
#include <string>

/*
** Handles cli flags and args
*/

struct Option {
  bool verbose = false;
  bool witness = false;

  std::optional<size_t> num_threads;
  std::optional<std::string> inputFile;
  std::optional<std::string> outputDir;
};

typedef std::function<void(Option &)> NoArgHandle;
const std::unordered_map<std::string, NoArgHandle> NoArgs{
    {"--verbose", [](Option &s) { s.verbose = true; }},
    {"-v", [](Option &s) { s.verbose = true; }},

    {"--witness", [](Option &s) { s.witness = true; }},
    {"-w", [](Option &s) { s.witness = true; }},
};

typedef std::function<void(Option &, const std::string &)> OneArgHandle;
const std::unordered_map<std::string, OneArgHandle> OneArgs{
    {"-o", [](Option &s, const std::string &out) { s.outputDir = out; }},
    {"--outputDir",
     [](Option &s, const std::string &out) { s.outputDir = out; }},

    {"-p",
     [](Option &s, const std::string &str) {
       try {
         uint32_t num = static_cast<size_t>(std::stoul(str));
         s.num_threads = num;
       } catch (const std::exception &e) {
         std::cerr << "Conversion failed: " << e.what() << std::endl;
         throw std::runtime_error{"Invalid argument for num_threads"};
       }
     }},
    {"--parallel",
     [](Option &s, const std::string &str) {
       try {
         uint32_t num = static_cast<size_t>(std::stoul(str));
         s.num_threads = num;
       } catch (const std::exception &e) {
         std::cerr << "Conversion failed: " << e.what() << std::endl;
         throw std::runtime_error{"Invalid argument for num_threads"};
       }
     }},
};

Option parseOptions(int argc, char *argv[]);

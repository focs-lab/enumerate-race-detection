#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

#include "../common/event.cpp"

std::unordered_map<std::string, EventType> eventTypeMapping = {
    {"Read", EventType::Read},   {"Write", EventType::Write},
    {"Acq", EventType::Acquire}, {"Rel", EventType::Release},
    {"Begin", EventType::Begin}, {"End", EventType::End},
    {"Fork", EventType::Fork},   {"Join", EventType::Join}};

std::unordered_map<std::string, uint32_t> varNameMapping;
uint32_t availVarId = 0;

std::unordered_map<std::string, uint32_t> lockNameMapping;
uint32_t availLockId = 0;

uint64_t parseLineToRawEvent(const std::string &line) {
  std::istringstream iss(line);
  std::string eventTypeStr, varName;
  uint32_t threadId, varValue, varId;

  if (!(iss >> eventTypeStr >> threadId >> varName >> varValue)) {
    throw std::runtime_error("Error parsing line: " + line);
  }

  EventType eventType = eventTypeMapping[eventTypeStr];

  if (eventType == EventType::Read || eventType == EventType::Write) {
    if (varNameMapping.find(varName) != varNameMapping.end()) {
      varId = varNameMapping[varName];
    } else {
      varId = varNameMapping[varName] = availVarId++;
    }
  } else if (eventType == EventType::Acquire ||
             eventType == EventType::Release) {
    if (lockNameMapping.find(varName) != lockNameMapping.end()) {
      varId = lockNameMapping[varName];
    } else {
      varId = lockNameMapping[varName] = availLockId++;
    }
  } else if (eventType == EventType::Fork || eventType == EventType::Join) {
    varId = std::stoul(varName);
  } else {
    varId = 0;
  }

  return Event::createRawEvent(eventType, threadId, varId, varValue);
}

bool convertToBinaryFile(std::string inputFilePath, std::string outputDir) {
  std::filesystem::path inputPath(inputFilePath);
  std::string filename = inputPath.filename().string();
  std::filesystem::path outputFilePath =
      std::filesystem::path(outputDir) / filename;

  std::ifstream inputFile(inputFilePath);
  std::ofstream outputFile(outputFilePath, std::ios::binary);

  if (!inputFile.is_open() || !outputFile.is_open()) {
    std::cerr << "Error opening files." << std::endl;
    return false;
  }

  std::string line;
  while (std::getline(inputFile, line)) {
    try {
      uint64_t rawEvent = parseLineToRawEvent(line);
      outputFile.write(reinterpret_cast<const char *>(&rawEvent),
                       sizeof(rawEvent));
    } catch (const std::runtime_error &e) {
      std::cerr << e.what() << std::endl;
      return false;
    }
  }

  inputFile.close();
  outputFile.close();

  return true;
}

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>

uint32_t get_random_int(uint32_t start, uint32_t end) {
  if (start > end) {
    throw std::invalid_argument("Start must be less than or equal to end.");
  }
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint32_t> dis(start, end - 1);

  return dis(gen);
}

double average(std::vector<uint32_t> nums) {
  return std::accumulate(nums.begin(), nums.end(), 0) / (double)nums.size();
}

void analyse_vector(std::vector<uint32_t> nums) {
  std::sort(nums.begin(), nums.end());
  uint32_t min = nums.front();
  uint32_t max = nums.back();
  double avg = average(nums);
  uint32_t quartile1 = nums[(size_t)(0.25 * nums.size())];
  uint32_t quartile3 = nums[(size_t)(0.75 * nums.size())];

  std::cout << "[" << min << ',' << quartile1 << ',' << avg << ',' << quartile3
            << ',' << max << ']' << std::endl;
}

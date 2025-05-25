#pragma once

#include <cstdint>
#include <vector>


extern std::vector<size_t> find_newlines_naive(const uint8_t* data, size_t size, size_t offset);
extern std::vector<size_t> find_newlines_sse2(const uint8_t* data, size_t size, size_t offset);
extern std::vector<size_t> find_newlines_avx2(const uint8_t* data, size_t size, size_t offset);
extern std::vector<size_t> find_newlines(const uint8_t* data, size_t total_size, size_t num_threads);

#pragma once

#include <cstdint>
#include <vector>

struct WorkUnit {
	std::vector<size_t> output;
	const uint8_t* data;
	size_t size;
	size_t offset;
	size_t longest_line {};

	~WorkUnit() {
	}
};

extern void find_newlines_avx2(WorkUnit& unit);
// extern std::vector<size_t> find_newlines(const uint8_t* data, size_t total_size, size_t num_threads);

#include <condition_variable>
#include <immintrin.h>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <thread>
#include "parser.h"
//
// static std::vector<size_t> find_newlines_naive(const uint8_t* data, size_t size, size_t offset) {
// 	std::vector<size_t> newline_positions;
// 	newline_positions.reserve(1024 * 1024);
//
// 	for (size_t i = 0; i < size; ++i) {
// 		if (data[i] == '\n') {
// 			newline_positions.push_back(offset + i + 1);
// 		}
// 	}
// 	return newline_positions;
// }
//
// static std::vector<size_t> find_newlines_sse2(const uint8_t* data, size_t size, size_t offset) {
// 	std::vector<size_t> newline_positions;
// 	newline_positions.reserve(1024 * 1024);
//
// 	const __m128i newline = _mm_set1_epi8('\n');
// 	size_t i = 0;
//
// 	// Process in 16-byte chunks
// 	for (; i + 16 <= size; i += 16) {
// 		__m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + i));
// 		__m128i cmp = _mm_cmpeq_epi8(chunk, newline);
// 		int mask = _mm_movemask_epi8(cmp);
//
// 		while (mask != 0) {
// 			int bit = __builtin_ctz(mask); // Find index of lowest set bit
// 			newline_positions.push_back(offset + i + bit + 1);
// 			mask &= (mask - 1); // Clear lowest set bit
// 		}
// 	}
//
// 	// Tail loop
// 	for (; i < size; ++i) {
// 		if (data[i] == '\n') {
// 			newline_positions.push_back(offset + i + 1);
// 		}
// 	}
//
// 	return newline_positions;
// }


void find_newlines_avx2(WorkUnit& unit) {
	const __m256i newline = _mm256_set1_epi8('\n');
	size_t i = 0;

	size_t prev_end = unit.offset - 1;
	size_t end;
	for (; i + 32 <= unit.size; i += 32) {
		__m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(unit.data + i));
		__m256i cmp = _mm256_cmpeq_epi8(chunk, newline);
		int mask = _mm256_movemask_epi8(cmp);

		while (mask != 0) {
			int bit = __builtin_ctz(mask);
			mask &= (mask - 1);
			end = unit.offset + i + bit;
			unit.output.push_back(end);
			unit.longest_line = std::max(unit.longest_line, end - prev_end);
			prev_end = end;
		}
	}

	// Tail loop
	for (; i < unit.size; ++i) {
		if (unit.data[i] == '\n') {
			end = unit.offset + i;
			unit.output.push_back(end);
			unit.longest_line = std::max(unit.longest_line, end - prev_end);
			prev_end = end;
		}
	}
}

void find_newlines_avx512(WorkUnit& unit) {
	const __m512i newline = _mm512_set1_epi8('\n');
	size_t i = 0;

	for (; i + 64 <= unit.size; i += 64) {
		__m512i chunk = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(unit.data + i));
		__mmask64 mask = _mm512_cmpeq_epi8_mask(chunk, newline); // 64-bit mask

		while (mask) {
			int bit = _tzcnt_u64(mask);  // Find first set bit
			unit.output.push_back(unit.offset + i + bit);
			mask &= (mask - 1); // Clear lowest set bit
		}
	}

	// Tail bytes
	for (; i < unit.size; ++i) {
		if (unit.data[i] == '\n') {
			unit.output.push_back(unit.offset + i);
		}
	}
}


// std::vector<size_t> find_newlines(const uint8_t* data, size_t total_size, size_t num_threads) {
// 	std::vector<size_t> newline_positions;
// 	newline_positions.reserve(1024 * 1024);
// 	newline_positions.push_back(0);
//
// 	size_t chunk_size = total_size / num_threads;
//
// 	std::vector<std::thread> threads(num_threads);
// 	std::vector<WorkUnit> units(num_threads);
//
// 	for (int i = 0; i < num_threads; ++i) {
// 		auto offset = i * chunk_size;
// 		size_t actual_size = (i == num_threads - 1) ? (total_size - offset) : chunk_size;
// 		units[i] = { {}, data + offset, actual_size, offset };
// 		threads[i] = std::thread(find_newlines_avx2, std::ref(units[i]));
// 	}
//
// 	for (auto& thread : threads) {
// 		thread.join();
// 	}
//
// 	for (const auto& unit : units) {
// 		newline_positions.insert(newline_positions.end(), unit.output.begin(), unit.output.end());
// 	}
//
// // 	// Parallelize the search using OpenMP
// // 	// TODO does this work? Currently has no effect on time
// // #pragma omp parallel for
// // 	for (int i = 0; i < num_threads; ++i) {
// // 		const uint8_t* chunk_start = data + i * chunk_size;
// // 		size_t actual_size = (i == num_threads - 1) ? (total_size - i * chunk_size) : chunk_size;
// // 		auto partial = find_newlines_avx2(chunk_start, actual_size, i * chunk_size);
// //
// // #pragma omp critical
// // 		newline_positions.insert(newline_positions.end(), partial.begin(), partial.end());
// // 	}
// 	return newline_positions;
// }

#include <immintrin.h>
#include <vector>
#include <cstdint>
#include <cstddef>

std::vector<size_t> find_newlines_naive(const uint8_t* data, size_t size, size_t offset) {
	std::vector<size_t> newline_positions;
	newline_positions.reserve(1024 * 1024);

	for (size_t i = 0; i < size; ++i) {
		if (data[i] == '\n') {
			newline_positions.push_back(offset + i + 1);
		}
	}
	return newline_positions;
}

std::vector<size_t> find_newlines_sse2(const uint8_t* data, size_t size, size_t offset) {
	std::vector<size_t> newline_positions;
	newline_positions.reserve(1024 * 1024);

	const __m128i newline = _mm_set1_epi8('\n');
	size_t i = 0;

	// Process in 16-byte chunks
	for (; i + 16 <= size; i += 16) {
		__m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + i));
		__m128i cmp = _mm_cmpeq_epi8(chunk, newline);
		int mask = _mm_movemask_epi8(cmp);

		while (mask != 0) {
			int bit = __builtin_ctz(mask); // Find index of lowest set bit
			newline_positions.push_back(offset + i + bit + 1);
			mask &= (mask - 1); // Clear lowest set bit
		}
	}

	// Tail loop
	for (; i < size; ++i) {
		if (data[i] == '\n') {
			newline_positions.push_back(offset + i + 1);
		}
	}

	return newline_positions;
}

std::vector<size_t> find_newlines_avx2(const uint8_t* data, size_t size, size_t offset) {
	std::vector<size_t> newline_positions;
	newline_positions.reserve(1024 * 1024);

	const __m256i newline = _mm256_set1_epi8('\n');
	size_t i = 0;

	for (; i + 32 <= size; i += 32) {
		__m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(data + i));
		__m256i cmp = _mm256_cmpeq_epi8(chunk, newline);
		int mask = _mm256_movemask_epi8(cmp);

		while (mask != 0) {
			int bit = __builtin_ctz(mask);
			newline_positions.push_back(offset + i + bit + 1);
			mask &= (mask - 1);
		}
	}

	// Tail loop
	for (; i < size; ++i) {
		if (data[i] == '\n') {
			newline_positions.push_back(offset + i + 1);
		}
	}

	return newline_positions;
}

std::vector<size_t> find_newlines(const uint8_t* data, size_t total_size, size_t num_threads) {
	std::vector<size_t> newline_positions;
	newline_positions.reserve(1024 * 1024);
	newline_positions.push_back(0);

	size_t chunk_size = total_size / num_threads;

	// Parallelize the search using OpenMP
	// TODO does this work? Currently has no effect on time
#pragma omp parallel for
	for (int i = 0; i < num_threads; ++i) {
		const uint8_t* chunk_start = data + i * chunk_size;
		size_t actual_size = (i == num_threads - 1) ? (total_size - i * chunk_size) : chunk_size;
		auto partial = find_newlines_avx2(chunk_start, actual_size, i * chunk_size);

#pragma omp critical
		newline_positions.insert(newline_positions.end(), partial.begin(), partial.end());
	}
	return newline_positions;
}

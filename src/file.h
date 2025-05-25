#pragma once

#include <cstdint>
#include <vector>
#ifdef WIN32
#include <windows.h>
#else
#endif

class File {
	const char *path_;
	size_t size_ = 0;
	const uint8_t *mapped_data_ = nullptr;
	const std::vector<uint8_t> tailed_data_;
#ifdef WIN32
	HANDLE hFile_ = INVALID_HANDLE_VALUE;
	HANDLE hMap_ = INVALID_HANDLE_VALUE;
#else
	int fd_ = -1;
#endif

public:
	File(const char *path);

	int open();
	void close();
	int mmap();

	size_t size() const {
		return size_;
	}
	const uint8_t *mapped_data() const {
		return mapped_data_;
	}
	const uint8_t *tailed_data() const {
		return tailed_data_.data();
	}
};

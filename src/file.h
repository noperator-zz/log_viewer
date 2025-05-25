#pragma once

#include <cstdint>
#ifdef WIN32
#include <windows.h>
#else
#endif

class File {
	const char *path_;
	size_t size_ = 0;
	const uint8_t *data_ = nullptr;
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
	const uint8_t *data() const {
		return data_;
	}
};

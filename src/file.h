#pragma once

#include <cstdint>
#ifdef WIN32
#include <windows.h>
#else
#endif

class File {
	const char *path_;
	size_t mapped_size_ {};
	const uint8_t *mapped_data_ {};
#ifdef WIN32
	HANDLE hFile_ = INVALID_HANDLE_VALUE;
	HANDLE hMap_ = INVALID_HANDLE_VALUE;
#else
	int fd_ = -1;
#endif


public:
	explicit File(const char *path);

	int open();
	size_t size() const;
	int mmap();
	void close();

	size_t mapped_size() const;
	const uint8_t *mapped_data() const;

	static void touch_pages(const uint8_t *addr, size_t size);
};

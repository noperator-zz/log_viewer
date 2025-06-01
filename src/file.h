#pragma once

#include <cstdint>
#include <vector>
#ifdef WIN32
#include <windows.h>
#else
#endif

class File {
	const char *path_;
	size_t mapped_size_ {};
	const uint8_t *mapped_data_ {};
	const std::vector<uint8_t> tailed_data_ {};
#ifdef WIN32
	HANDLE hFile_ = INVALID_HANDLE_VALUE;
	HANDLE hMap_ = INVALID_HANDLE_VALUE;
#else
	int fd_ = -1;
#endif

	int mmap();

public:
	explicit File(const char *path);

	int open();
	void close();

	size_t mapped_size() const;
	const uint8_t *mapped_data() const;
	const uint8_t *tailed_data() const;

	static void touch_pages(const uint8_t *addr, size_t size);
};

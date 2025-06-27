#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef WIN32
#include <windows.h>
#include <memoryapi.h>
#else
#include <sys/mman.h>
#endif

#include "file.h"

File::File(const char *path) : path_(path) {
}

int File::open() {
#ifdef WIN32
	if (hFile_ != INVALID_HANDLE_VALUE) {
		return 0;
	}
	hFile_ = CreateFileA(
		path_,
		GENERIC_READ,             // desired access
		FILE_SHARE_READ | FILE_SHARE_WRITE,          // share mode
		NULL,                     // security
		OPEN_EXISTING,            // creation disposition
		FILE_ATTRIBUTE_NORMAL,    // flags
		NULL                      // template file
	);

	if (hFile_ == INVALID_HANDLE_VALUE) {
		return -1;
	}
#else
	if (fd_ != -1) {
		return 0;
	}

	fd_ = ::open(path_, O_RDONLY);
	if (fd_ == -1) {
		return -1;
	}
#endif
	return 0;
}

size_t File::size() const {
#ifdef WIN32
	LARGE_INTEGER fileSize;
	if (!GetFileSizeEx(hFile_, &fileSize)) {
		return -1;
	}

	return fileSize.QuadPart;
#else
	struct stat sb;
	if (fstat(fd_, &sb) == -1) {
		return -2;
	}

	return sb.st_size;
#endif
}

int File::mmap() {
	size_t current_size = size();

	if (current_size == mapped_size_) {
		return 0; // Already mapped
	}

	mapped_size_ = current_size;

#ifdef WIN32
	if (hMap_ != INVALID_HANDLE_VALUE) {
		UnmapViewOfFile(hMap_);
		CloseHandle(hMap_);
		hMap_ = INVALID_HANDLE_VALUE;
	}

	hMap_ = CreateFileMappingA(
		hFile_,                    // file handle
		NULL,                     // security
		PAGE_READONLY,            // protection
		mapped_size_ >> 32,         // maximum size high
		mapped_size_ & 0xFFFFFFFF,       // maximum size low
		NULL                      // mapping name
	);

	if (!hMap_) {
		return -1;
	}

	mapped_data_ = (const uint8_t*)MapViewOfFile(
		hMap_,                     // mapping handle
		FILE_MAP_READ,            // desired access
		0, 0,                     // offset
		0                         // size (0 = full file)
	);

	if (!mapped_data_) {
		CloseHandle(hMap_);
		return -2;
	}
#else
	if (mapped_data_ && mapped_data_ != MAP_FAILED) {
		::munmap((void*)mapped_data_, mapped_size_);
		mapped_data_ = nullptr;
	}

	mapped_data_ = (const uint8_t*)::mmap(NULL, mapped_size_, PROT_READ, MAP_SHARED, fd_, 0);
	if (mapped_data_ == MAP_FAILED) {
		return -1;
	}
#endif
	return 0;
}

void File::close() {
#ifdef WIN32
	if (hMap_ != INVALID_HANDLE_VALUE) {
		UnmapViewOfFile(hMap_);
		CloseHandle(hMap_);
		hMap_ = INVALID_HANDLE_VALUE;
	}
	if (hFile_ != INVALID_HANDLE_VALUE) {
		CloseHandle(hFile_);
		hFile_ = INVALID_HANDLE_VALUE;
	}
#else
	if (mapped_data_ && mapped_data_ != MAP_FAILED) {
		::munmap((void*)mapped_data_, mapped_size_);
		mapped_data_ = nullptr;
	}
	::close(fd_);
	fd_ = -1;
#endif
}

size_t File::mapped_size() const {
	return mapped_size_;
}

const uint8_t *File::mapped_data() const {
	return mapped_data_;
}

void File::touch_pages(const uint8_t *addr, size_t size) {
#ifdef WIN32
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	size_t page_size = si.dwPageSize;

	WIN32_MEMORY_RANGE_ENTRY range;
	range.VirtualAddress = (void*)addr;
	range.NumberOfBytes = size;
	PrefetchVirtualMemory(GetCurrentProcess(), 1, &range, 0);
#else
    size_t page_size = sysconf(_SC_PAGESIZE);
	madvise(addr, length, MADV_WILLNEED);
#endif
	// for (size_t i = 0; i < size; i += page_size) {
	// 	const volatile uint8_t dummy = addr[i];  // force page-in
	// 	(void)dummy;
	// }
}

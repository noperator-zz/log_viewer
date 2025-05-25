#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef WIN32
#include <windows.h>
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
		FILE_SHARE_READ,          // share mode
		NULL,                     // security
		OPEN_EXISTING,            // creation disposition
		FILE_ATTRIBUTE_NORMAL,    // flags
		NULL                      // template file
	);

	if (hFile_ == INVALID_HANDLE_VALUE) {
		return -1;
	}

	LARGE_INTEGER fileSize;
	if (!GetFileSizeEx(hFile_, &fileSize)) {
		return -1;
	}

	size_ = fileSize.QuadPart;
#else
	if (fd_ != -1) {
		return 0;
	}

	fd_ = ::open(path_, O_RDONLY);
	if (fd_ == -1) {
		return -1;
	}

	struct stat sb;
	if (fstat(fd_, &sb) == -1) {
		return -2;
	}
	size_ = sb.st_size;
#endif
	return 0;
}

int File::mmap() {
#ifdef WIN32
	hMap_ = CreateFileMappingA(
		hFile_,                    // file handle
		NULL,                     // security
		PAGE_READONLY,            // protection
		0, 0,                     // max size (0 = full file)
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
	mapped_data_ = (const uint8_t*)::mmap(NULL, size_, PROT_READ, MAP_SHARED, fd_, 0);
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
		::munmap((void*)mapped_data_, size_);
		mapped_data_ = nullptr;
	}
	::close(fd_);
	fd_ = -1;
#endif
}

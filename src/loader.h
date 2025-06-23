#pragma once
#include <mutex>

#include "file.h"
#include "worker.h"

class Loader {
	File file_;
	WorkerPool workers_;
	std::mutex &mtx_;
	std::vector<size_t> line_starts_ {};
	// NOTE: This length includes the newline character. It's only used for scroll bar size calculations, so fine for now.
	size_t longest_line_ {};
	void worker(const Event &quit);
	void load_initial();
	void load_tail();

public:
	Loader(File &&file, size_t num_workers, std::mutex &mutex);

	~Loader();

	std::thread start(const Event &quit);
	void update(std::vector<size_t> &line_starts, size_t &longest_line, const uint8_t *&data);
};

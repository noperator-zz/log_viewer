#pragma once

#include <mutex>
#include <functional>
#include <thread>

#include "file.h"
#include "worker.h"

class Loader {
	File file_;
	WorkerPool workers_;
	std::function<void()> on_data_;
	std::function<void()> on_remap_;
	std::mutex mtx_ {};
	std::vector<size_t> line_starts_ {};
	// NOTE: This length includes the newline character. It's only used for scroll bar size calculations, so fine for now.
	size_t longest_line_ {};
	std::thread thread_ {};
	Event quit_ {};
	void worker();
	void load_initial();
	void load_tail();

public:
	Loader(File &&file, size_t num_workers, std::function<void()> &&on_data, std::function<void()> &&on_remap);

	~Loader();

	std::mutex &mutex();
	void start();
	void stop();

	const uint8_t *get_data() const;
	size_t get_size() const;
	void get(std::vector<size_t> &line_starts, size_t &longest_line, const uint8_t *&data);
};

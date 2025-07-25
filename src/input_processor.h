#pragma once

#include <mutex>
#include <functional>
#include <thread>
#include <hs/hs.h>

#include "dataset.h"
#include "dynarray.h"
#include "file.h"
#include "worker.h"

class InputProcessor {
	File file_;
	Dataset &dataset_;
	std::function<void()> on_data_;
	hs_database_t * db_ {};
	hs_scratch_t * scratch_ {};
	hs_stream_t * stream_ {};
	TracyLockable(std::mutex, mtx_);
	size_t prev_start_ {};
	dynarray<size_t> chunk_results_ {};
	dynarray<size_t> line_starts_ {};
	// NOTE: This length includes the newline character. It's only used for scroll bar size calculations, so fine for now.
	size_t unsafe_longest_line_ {};
	size_t longest_line_ {};
	std::thread thread_ {};
	Event quit_ {};

	void quit();
	void worker();
	void load_tail();

	static int event_handler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags, void *context);
	int event_handler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags);

	InputProcessor() = delete;
	// diable copy and move
	InputProcessor(const InputProcessor &) = delete;
	InputProcessor &operator=(const InputProcessor &) = delete;

	InputProcessor(InputProcessor &&) = delete;
	InputProcessor &operator=(InputProcessor &&) = delete;

public:
	InputProcessor(File &&file, Dataset &dataset, std::function<void()> &&on_data);
	~InputProcessor();

	int start();
	void stop();

	void get(dynarray<size_t> &line_starts, size_t &longest_line);
};

#include "loader.h"

#include <cassert>

#include "parser.h"
#include "util.h"
#include <hs/hs.h>

using namespace std::chrono;

Loader::Loader(File &&file, std::function<void(const uint8_t *data, size_t size)> &&on_data,
	std::function<void()> &&on_unmap)
	: file_(std::move(file)), on_data_(std::move(on_data)), on_unmap_(std::move(on_unmap)) {
}

Loader::~Loader() {
	{
		Timeit t("Loader::~Loader()");
		quit();
	}

	if (stream_) hs_close_stream(stream_, nullptr, nullptr, nullptr);
	if (scratch_) hs_free_scratch(scratch_);
	if (db_) hs_free_database(db_);
}

std::mutex &Loader::mutex() {
	return mtx_;
}

int Loader::start() {
	hs_compile_error_t *compile_err;
	hs_error_t err;

	err = hs_compile_lit("\n", 0, 1, HS_MODE_STREAM, NULL, &db_, &compile_err);
	if (err != HS_SUCCESS) {
		fprintf(stderr, "ERROR: Unable to compile pattern: %s\n", compile_err->message);
		hs_free_compile_error(compile_err);
		return -1;
	}

	err = hs_alloc_scratch(db_, &scratch_);
	if (err != HS_SUCCESS) {
		fprintf(stderr, "ERROR: Unable to allocate scratch space. Exiting.\n");
		return -2;
	}

	err = hs_open_stream(db_, 0, &stream_);
	if (err != HS_SUCCESS) {
		fprintf(stderr, "ERROR: Unable to open stream. Exiting.\n");
		return -3;
	}

	thread_ = std::thread(&Loader::worker, this);
	return 0;
}

void Loader::stop() {
	quit_.set();
	if (thread_.joinable()) {
		thread_.join();
	}
}

void Loader::get(dynarray<size_t> &line_starts, size_t &longest_line, const uint8_t *&data) {
	line_starts.extend(line_starts_);
	line_starts_.clear();
	longest_line = longest_line_;
	data = file_.mapped_data();
}

void Loader::quit() {
	quit_.set();
	if (thread_.joinable()) {
		thread_.join();
	}
}

void Loader::worker() {
	{
		Timeit timeit("File Open");
		if (file_.open() != 0) {
			std::cerr << "Failed to open file_\n";
			// TODO better error handling
			return;
		}
	}

	while (1) {
		load_tail();
		if (quit_.wait(1000ms)) {
			break;
		}
	}
}

int Loader::event_handler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags, void *context) {
	return static_cast<Loader *>(context)->event_handler(id, from, to, flags);
}

int Loader::event_handler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags) {
	// This is faster and uses less memory than enabling the HS_FLAG_SOM_LEFTMOST flag
	from = to - 1;

	chunk_results_.push_back(from);
	size_t line_len = from - prev_start_;
	prev_start_ = from;
	longest_line_ = std::max(longest_line_, line_len);
	return 0; // Continue matching
}

void Loader::load_tail() {
	static constexpr size_t CHUNK_SIZE = 1ULL * 1024 * 1024;

	auto prev_size = file_.mapped_size();
	auto new_size = file_.size();

	if (new_size <= prev_size) {
		// No new data to load
		return;
	}

	if (on_unmap_) {
		on_unmap_();
	}

	{
		// remapping can change the data address, so we need to lock the mutex
		std::lock_guard lock(mtx_);

		// Timeit timeit("File remap");
		if (file_.mmap() != 0) {
			std::cerr << "Failed to map file_\n";
			// TODO better error handling
			return;
		}
		// timeit.stop();
		// std::cout << "File remapped: " << prev_size << " B -> " << file_.mapped_size() << " B\n";

		if (on_data_) {
			on_data_(file_.mapped_data(), file_.mapped_size());
		}
	}

	size_t total_size = new_size - prev_size;
	std::cout << "Loading " << total_size << " B\n";
	Timeit load_timeit("Load");

	chunk_results_.clear();
	if (prev_size == 0) {
		// If this is the first time we are loading the file, we need to add a starting point
		prev_start_ = 0;
		chunk_results_.push_back(0);
	}

	for (size_t offset = 0; offset < total_size; offset += CHUNK_SIZE) {
		size_t chunk_size = std::min(total_size - offset, CHUNK_SIZE);
		hs_error_t err = hs_scan_stream(stream_, (const char*)file_.mapped_data() + prev_size + offset, chunk_size, 0,
			scratch_, event_handler, this);

		assert(err == HS_SUCCESS);

		{
			std::lock_guard lock(mtx_);
			line_starts_.extend(chunk_results_);
		}
		chunk_results_.clear();
	}
	load_timeit.stop();

	if (on_data_) {
		on_data_(file_.mapped_data(), file_.mapped_size());
	}
}

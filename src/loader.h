#pragma once
#include <mutex>

#include "file.h"
#include "worker.h"

class Loader {
public:
	enum class State {
		INIT,
		FIRST_READY,
		LOAD_TAIL,
	};

	std::mutex mtx {};
private:
	File file_;
	WorkerPool workers_;
	State state_ {};
	std::vector<size_t> line_ends_ {};
	size_t mapped_lines_ {};
	// NOTE: This length includes the newline character. It's only used for scroll bar size calculations, so fine for now.
	size_t longest_line_ {};

	void worker(const Event &quit);
	void load_inital();
	void load_tail();

public:
	Loader(File &&file, size_t num_workers);
	~Loader();

	struct Snapshot {
		State state;
		size_t longest_line;
		const std::vector<size_t> &line_ends;
		const uint8_t *data;
	};

	std::thread start(const Event &quit);
	[[nodiscard]] Snapshot snapshot(const std::lock_guard<std::mutex> &lock) const;
};

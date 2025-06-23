#pragma once
#include <mutex>

#include "worker.h"

class Finder {
	class Job {
		enum class State {
			Start,
			Active,
		};

		struct Result {
			size_t start;
			size_t end;
		};
		// std::vector<Result> unsafe_results_ {};

	public:
		State state_ {};
		std::vector<Result> results_ {};

		void reset() {
			state_ = State::Start;
			results_.clear();
		}
	};

	WorkerPool workers_;
	std::mutex &mtx_;
	std::unordered_map<const void*, Job> results_ {};
	// void worker(const Event &quit);

public:
	Finder(size_t num_workers, std::mutex &mutex);

	~Finder();

	// std::thread start(const Event &quit);
	void add_data(const uint8_t *data, size_t len);
	void submit(const void* ctx, std::string_view pattern, int flags);
	void remove(const void* ctx);
};


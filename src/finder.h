#pragma once
#include <mutex>
#include <hs/hs_runtime.h>

#include "worker.h"

class Finder {
	class Job {
		friend class Finder;
		enum class State {
			New,
			Idle,
			Active,
			Abort,
		};

		struct Result {
			size_t start;
			size_t end;
		};
		// std::vector<Result> unsafe_results_ {};

		std::thread thread_;
		std::string_view pattern_;
		int flags_;

		hs_database_t *db_ {};
		hs_scratch_t *scratch_ {};
		hs_stream_t *stream_ {};
		State state_ {};
		std::condition_variable in_cv_ {};
		std::vector<Result> chunk_results_ {};

		void abort(std::mutex &pool_mtx, std::condition_variable &pool_cv);
		static int event_handler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags, void *context);
		int event_handler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags);
		void search(std::mutex &pool_mtx, std::condition_variable &pool_cv, bool &quit, const uint8_t *data, size_t length, const void* ctx, std::string_view pattern, int flags);
		void worker();
	public:
		std::vector<Result> results_ {};

		Job() = delete;
		Job(std::string_view pattern, int flags);

		void reset() {
			state_ = {};
			results_.clear();
		}
	};

	// WorkerPool workers_;
	// std::mutex &mtx_;
	std::unordered_map<const void*, std::unique_ptr<Job>> jobs_ {};
	// void worker(const Event &quit);

public:
	Finder(size_t num_workers, std::mutex &mutex);

	~Finder();

	// std::thread start(const Event &quit);
	void add_data(const uint8_t *data, size_t len);
	void submit(const void* ctx, std::string_view pattern, int flags);
	void remove(const void* ctx);
};


#pragma once
#include <mutex>
#include <hs/hs_runtime.h>

#include "worker.h"

class Finder {
	class Job {
	public:
		enum class Status {
			kOK,
			kQUIT,
			kERROR,
		};

	private:
		struct Result {
			size_t start;
			size_t end;
		};

		struct QueueItem {
			const uint8_t *data;
			size_t length;
		};

		std::thread thread_;
		std::string_view pattern_;
		int flags_;
		hs_database_t *db_;
		hs_scratch_t *scratch_;
		hs_stream_t *stream_;

		std::condition_variable cv_ {};
		std::queue<QueueItem> queue_ {};
		std::atomic<bool> quit_ {};
		size_t stream_pos_ {};
		std::atomic<Status> status_ {};

		// std::condition_variable in_cv_ {};
		std::vector<Result> chunk_results_ {};

		static int event_handler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags, void *context);
		int event_handler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags);
		void worker();
		void quit();

		Job() = delete;
		Job(std::string_view pattern, int flags, hs_database_t *db_, hs_scratch_t *scratch_, hs_stream_t *stream_);
		// diable copy and move
		Job(const Job &) = delete;
		Job &operator=(const Job &) = delete;

		Job(Job &&) = delete;
		Job &operator=(Job &&) = delete;

	public:
		std::mutex mtx_ {};
		std::vector<Result> results_ {};

		static std::unique_ptr<Job> && create(std::string_view pattern, int flags, int &err);
		~Job();

		std::mutex &mutex();
		void update_data(const uint8_t *data, size_t len);
		Status status() const;
	};

	std::mutex mtx_ {};
	std::unordered_map<const void*, std::unique_ptr<Job>> jobs_ {};
	const uint8_t *latest_data_ {};
	size_t latest_length_ {};

public:
	Finder() = default;
	~Finder();

	void stop();

	std::mutex &mutex();
	const std::unordered_map<const void*, std::unique_ptr<Job>> & jobs() const;
	void update_data(const uint8_t *data, size_t len);
	[[nodiscard]] int submit(const void* ctx, std::string_view pattern, int flags);
	void remove(const void* ctx);
};


#pragma once
#include <mutex>
#include <shared_mutex>
#include <hs/hs_runtime.h>

#include "dynarray.h"
#include "worker.h"

class Finder {
	class Job {
	public:
		enum class Status {
			kOK,
			kQUIT,
			kERROR,
		};

		struct Dataset {
			std::atomic_flag update_pending_ {};

			std::shared_mutex mtx {};
			const uint8_t *data {};
			size_t length {};
		};

	private:
		struct Result {
			size_t start;
			size_t end;
		};

		std::thread thread_;
		std::condition_variable_any &update_cv_;
		Dataset &dataset_;
		const std::string_view pattern_;
		const int flags_;
		hs_database_t * const db_;
		hs_scratch_t * const scratch_;
		hs_stream_t * const stream_;

		std::mutex result_mtx_ {};
		std::atomic_flag quit_ {};
		size_t stream_pos_ {};
		std::atomic<Status> status_ {};

		dynarray<Result> chunk_results_ {};

		static int event_handler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags, void *context);
		int event_handler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags);
		void worker();
		void quit();

		Job() = delete;
		Job(std::condition_variable_any &update_cv, Dataset &dataset, std::string_view pattern, int flags,
			hs_database_t *db_, hs_scratch_t *scratch_, hs_stream_t *stream_);
		// diable copy and move
		Job(const Job &) = delete;
		Job &operator=(const Job &) = delete;

		Job(Job &&) = delete;
		Job &operator=(Job &&) = delete;

	public:
		dynarray<Result> results_ {};

		static std::unique_ptr<Job> create(std::condition_variable_any &update_cv, Dataset &dataset,
			std::string_view pattern, int flags, int &err);
		~Job();

		std::mutex &result_mtx();
		Status status() const;
	};

	std::condition_variable_any update_cv_ {};
	Job::Dataset dataset_ {};

	std::mutex jobs_mtx_ {};
	std::unordered_map<const void*, std::unique_ptr<Job>> jobs_ {};

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


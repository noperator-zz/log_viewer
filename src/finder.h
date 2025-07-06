#pragma once
#include <mutex>
#include <shared_mutex>
#include <hs/hs_runtime.h>

#include "dataset.h"
#include "dynarray.h"
#include "worker.h"

class Finder {
public:
	class Job {
	public:
		enum class Status {
			kOK,
			kQUIT,
			kERROR,
		};

		struct Result {
			size_t start;
			size_t end;

			bool operator<(const Result &other) const {
				return start < other.start;
			}

			bool operator<(size_t pos) const {
				return start < pos;
			}
		};

	private:
		std::thread thread_;
		std::mutex &result_mtx_;
		Dataset &dataset_;
		std::function<void(void*, size_t)> on_result_;
		void *ctx_;
		const std::string_view pattern_;
		const int flags_;
		hs_database_t * const db_;
		hs_scratch_t * const scratch_;
		hs_stream_t * const stream_;

		std::atomic_flag quit_ {};
		size_t stream_pos_ {};
		std::atomic<Status> status_ {};

		dynarray<Result> chunk_results_ {};
		dynarray<Result> results_ {};
		size_t last_report_ {};

		static int event_handler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags, void *context);
		int event_handler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags);
		void worker();
		void quit();

		Job() = delete;
		Job(std::mutex &result_mtx, Dataset &dataset, std::function<void(void*, size_t)> &&on_result, void* ctx,
			std::string_view pattern, int flags, hs_database_t *db, hs_scratch_t *scratch, hs_stream_t *stream);
		// diable copy and move
		Job(const Job &) = delete;
		Job &operator=(const Job &) = delete;

		Job(Job &&) = delete;
		Job &operator=(Job &&) = delete;

	public:
		~Job();
		static std::unique_ptr<Job> create(std::mutex &results_mtx, Dataset &dataset, std::function<void(void*, size_t)> &&on_result,
			void* ctx, std::string_view pattern, int flags, int &err);

		const dynarray<Result> &results() const { return results_; }
		Status status() const;
	};

	class User {
		friend class Finder;

		const Finder &finder_;
		std::scoped_lock<std::mutex, std::mutex> lock_;

		User(const Finder &finder) : finder_(finder), lock_(finder.jobs_mtx_, finder.results_mtx_) {}

		User() = delete;
		User(const User &) = delete;
		User &operator=(const User &) = delete;
		User(User &&) = delete;
		User &operator=(User &&) = delete;

	public:
		~User() = default;
		const std::unordered_map<void*, std::unique_ptr<Job>> & jobs() const { return finder_.jobs_; }
	};

	Dataset &dataset_;

	mutable std::mutex jobs_mtx_ {};
	mutable std::mutex results_mtx_ {};
	std::unordered_map<void*, std::unique_ptr<Job>> jobs_ {};

public:
	Finder(Dataset &dataset);
	~Finder();

	void stop();

	[[nodiscard]] int submit(void* ctx, std::function<void(void*, size_t)> &&on_result, std::string_view pattern, int flags);
	void remove(void* ctx);
	User user() const {	return User(*this);	}
};


#pragma once
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>

class Event {
	std::mutex _own_mtx_ {};
	std::condition_variable _own_cv_ {};

	std::mutex &mtx_;
	std::condition_variable &cv_;
	bool triggered_ {};

public:
	Event(std::mutex &mtx, std::condition_variable &cv) : mtx_(mtx), cv_(cv) {}
	Event() : mtx_(_own_mtx_), cv_(_own_cv_) {}
	void set();
	bool wait(std::chrono::milliseconds timeout = std::chrono::milliseconds::max()) const;
	bool is_set() const;
};

class WorkerPool {
	// friend class Worker;
	// class Worker {
	// 	std::mutex &mtx_;
	// 	std::condition_variable &cv_;
	// 	std::thread thread_;
	//
	// public:
	// 	Worker() = delete;
	// 	Worker(WorkerPool &pool);
	// 	void join();
	// };

public:
	using job_t = std::function<void(const std::mutex &pool_mtx, const std::condition_variable &pool_cv, const bool &pool_quit)>;
	std::mutex mtx_ {};
	std::condition_variable cv_ {};

private:
	// std::vector<Worker> workers_;
	std::vector<std::thread> workers_;
	std::queue<job_t> queue_ {};
	bool quit_ {};
	// Event quit_ {mtx_, cv_};
	std::atomic<size_t> active_jobs_ {};

	void worker();

public:
	WorkerPool(size_t num_workers);
	~WorkerPool();
	void push(job_t job);
	void close();
	void wait_free(size_t num_jobs);
};


#include "worker.h"


void Event::set() {
	{
		std::lock_guard lock(mtx_);
		triggered_ = true;
	}
	cv_.notify_all();
}

bool Event::wait(const std::chrono::milliseconds timeout) const {
	std::unique_lock lock(mtx_);
	if (timeout == std::chrono::milliseconds::max()) {
		cv_.wait(lock, [this] { return triggered_; });
		return true;
	}
	return cv_.wait_for(lock, timeout, [this] { return triggered_; });
}

bool Event::is_set() const {
	return triggered_;
}

WorkerPool::WorkerPool(size_t num_workers) {
	for (size_t i = 0; i < num_workers; ++i) {
		workers_.emplace_back(&WorkerPool::worker, this);
	}
}

WorkerPool::~WorkerPool() {
	close();
}

void WorkerPool::push(std::function<void(const Event &)> job) {
	{
		std::lock_guard lock(mtx_);
		++active_jobs_;
		queue_.push(std::move(job));
	}
	cv_.notify_one();
}

void WorkerPool::close() {
	quit_.set();
	cv_.notify_all(); // Wake up all workers to exit
	for (auto& worker : workers_) {
		if (worker.joinable()) {
			worker.join();
		}
	}
}

void WorkerPool::wait_free(size_t num_free) {
	size_t num_active = workers_.size() - num_free;
	std::unique_lock lock(mtx_);
	cv_.wait(lock, [this, num_active] { return active_jobs_ <= num_active; });
}

void WorkerPool::worker() {
	job_t job;
	while (true) {
		{
			std::unique_lock lock(mtx_);
			cv_.wait(lock, [this] { return quit_.is_set() || !queue_.empty(); });

			if (quit_.is_set() && queue_.empty()) {
				return; // Exit if quit flag is set and queue is empty
			}

			job = std::move(queue_.front());
			queue_.pop();
		}

		job(quit_);

		{
			std::lock_guard lock(mtx_);
			--active_jobs_;
		}
		cv_.notify_all();
	}
}

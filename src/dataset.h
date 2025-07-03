#pragma once
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <shared_mutex>
#include <functional>

#include "util.h"

class Dataset {
	class Updater {
		friend class Dataset;

		Dataset &dataset_;
		Timeit t_ {"Dataset::~Updater"};

		Updater(Dataset &dataset) : dataset_(dataset) {
			Timeit t {"Dataset::Updater"};
			dataset_.invalidate();
		}

		Updater() = delete;
		Updater(const Updater &) = delete;
		Updater &operator=(const Updater &) = delete;
		Updater(Updater &&) = delete;
		Updater &operator=(Updater &&) = delete;

	public:
		~Updater() {
			dataset_.update();
		}
		void set(const uint8_t *data, size_t length) const {
			dataset_.data_ = data;
			dataset_.length_ = length;
		}
	};

public:
	class User {
		friend class Dataset;

		const Dataset &dataset_;
		std::shared_lock<std::shared_mutex> lock_;
		// Timeit ctor_ {"Dataset::User"};
		// Timeit dtor_ {"Dataset::~User"};

		User(const Dataset &dataset) : User(dataset, std::shared_lock(dataset.mtx_)) {}
		User(const Dataset &dataset, std::shared_lock<std::shared_mutex> &&lock_) : dataset_(dataset), lock_(std::move(lock_)) {
			// ctor_.stop();
		}


		User() = delete;
		User(const User &) = delete;
		User &operator=(const User &) = delete;
		User(User &&) = delete;
		User &operator=(User &&) = delete;

	public:
		~User() = default;
		const uint8_t *data() const { return dataset_.data_; }
		size_t length() const { return dataset_.length_; }
	};

private:
	std::atomic_flag update_pending_ {};

	std::function<void()> on_data_ {};
	std::function<void()> on_unmap_ {};
	mutable std::shared_mutex mtx_ {};
	std::condition_variable_any update_cv_ {};
	const uint8_t *data_ {};
	size_t length_ {};

	void invalidate() {
		// NOTE: This tells active users of the dataset that an update is pending, so they should release the lock
		update_pending_.test_and_set();
		if (on_unmap_) {
			on_unmap_();
		}
		mtx_.lock();
	}

	void update() {
		update_pending_.clear();
		mtx_.unlock();
		update_cv_.notify_all();
		if (on_data_) {
			on_data_();
		}
	}

	Dataset() = delete;
	Dataset(const Dataset &) = delete;
	Dataset &operator=(const Dataset &) = delete;
	Dataset(Dataset &&) = delete;
	Dataset &operator=(Dataset &&) = delete;

public:
	Dataset(std::function<void()> &&on_data, std::function<void()> &&on_unmap) :
		on_data_(std::move(on_data)), on_unmap_(std::move(on_unmap)) {
	}
	~Dataset() = default;

	void notify() { update_cv_.notify_all(); }
	bool is_update_pending() const { return update_pending_.test();	}
	Updater updater() {	return Updater(*this);}
	User user() const {	return User(*this);	}

	User wait() {
		std::shared_lock lock(mtx_);
		update_cv_.wait(lock);
		return {*this, std::move(lock)};
	}

	template<typename Predicate>
	User wait(Predicate pred) {
		std::shared_lock lock(mtx_);
		while (!pred(length_))
			update_cv_.wait(lock);
		return {*this, std::move(lock)};
	}
};

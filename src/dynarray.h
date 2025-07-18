#pragma once
#include <cstring>
#include <type_traits>
#include <utility>

#include "util.h"

template<typename T>
class dynarray {
	struct DummyLock {
		constexpr DummyLock() {}
		constexpr void lock() {}
		constexpr void unlock() {}
	};

	T* data_ = nullptr;
	size_t size_ = 0;
	size_t capacity_ = 0;
	// TODO minimum capacity setting

	// diable copy
	dynarray(const dynarray &) = delete;
	dynarray &operator=(const dynarray &) = delete;

	void clear() {
		if constexpr (!std::is_trivially_destructible_v<T>) {
			for (size_t i = 0; i < size_; ++i) {
				data_[i].~T();
			}
		}
		size_ = 0;
	}

	void release() {
		clear();
		capacity_ = 0;
		::operator delete(data_);
		data_ = nullptr;
	}

public:
	typedef T* iterator;
	typedef const T* const_iterator;

	dynarray() = default;
	dynarray(size_t n) {
		reserve(n);
	}

	dynarray(dynarray && o) noexcept
		: data_(o.data_), size_(o.size_), capacity_(o.capacity_) {
		o.data_ = nullptr;
		o.size_ = 0;
		o.capacity_ = 0;
	}
	dynarray &operator=(dynarray && o) noexcept {
		if (this != &o) {
			release();
			data_ = o.data_;
			size_ = o.size_;
			capacity_ = o.capacity_;
			o.data_ = nullptr;
			o.size_ = 0;
			o.capacity_ = 0;
		}
		return *this;
	}

	void reserve(const size_t n) {
		if (n <= capacity_) {
			return;
		}
		DummyLock dummy;
		reserve(dummy, n);
	}

	template<typename Lock>
	void reserve(Lock &lock, const size_t n) {
		if (n <= capacity_) {
			return;
		}
		size_t new_capacity = capacity_;
		while (new_capacity < n) {
			if (new_capacity == 0) {
				new_capacity = 1;
			} else {
				new_capacity *= 2; // Double the capacity
			}
		}

		if (new_capacity > capacity_) {
			lock.unlock();
			Timeit t("dynarray::reserve");
			T* new_data = static_cast<T*>(::operator new(sizeof(T) * new_capacity));
			std::memcpy(new_data, data_, sizeof(T) * size_);
			T* old_data = data_;

			lock.lock();
			data_ = new_data;
			capacity_ = new_capacity;

			lock.unlock();
			::operator delete(old_data);

			t.stop();
			lock.lock();
		}
	}

	void resize_uninitialized(const size_t n) {
		DummyLock dummy;
		resize_uninitialized(dummy, n);
	}

	template<typename Lock>
	void resize_uninitialized(Lock &lock, const size_t n) {
		reserve(lock, n);
		size_ = n;
	}

	~dynarray() {
		release();
	}

	T* data() { return data_; }
	const T* data() const { return data_; }
	size_t size() const { return size_; }
	bool empty() const { return size_ == 0; }
	T& operator[](size_t index) { return data_[index]; }
	const T& operator[](size_t index) const { return data_[index]; }

	iterator begin() { return data_; }
	iterator end() { return data_ + size_; }
	const_iterator begin() const { return data_; }
	const_iterator end() const { return data_ + size_; }

	T& front() { return data_[0]; }
	const T& front() const { return data_[0]; }
	T& back() { return data_[size_ - 1]; }
	const T& back() const { return data_[size_ - 1]; }

	void push_back(const T &value) {
		DummyLock dummy;
		push_back(dummy, value);
	}

	template<typename Lock>
	void push_back(Lock &lock, const T value) {
		reserve(lock, size_ + 1);
		data_[size_++] = value;
	}

	template<typename... _Args>
	void emplace_back(_Args&&... __args) {
		DummyLock dummy;
		emplace_back_locked(dummy, std::forward<_Args>(__args)...);
	}

	template<typename Lock, typename... _Args>
	void emplace_back_locked(Lock &lock, _Args&&... __args) {
		reserve(lock, size_ + 1);
		new (&data_[size_++]) T(std::forward<_Args>(__args)...);
	}

	void extend(const dynarray& other) {
		DummyLock dummy;
		extend(dummy, other);
	}

	template<typename Lock>
	void extend(Lock &lock, const dynarray& other) {
		reserve(lock, size_ + other.size_);
		std::memcpy(data_ + size_, other.data_, sizeof(T) * other.size_);
		size_ += other.size_;
	}
};

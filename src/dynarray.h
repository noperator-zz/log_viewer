#pragma once
#include <cstring>
#include <type_traits>
#include <utility>

template<typename T>
class dynarray {
public:
	typedef T* iterator;
	typedef const T* const_iterator;

	dynarray() = default;

	void reserve(const size_t n) {
		size_t new_capacity = capacity_;
		while (new_capacity < n) {
			if (new_capacity == 0) {
				new_capacity = 1;
			} else {
				new_capacity *= 2; // Double the capacity
			}
		}

		if (new_capacity > capacity_) {
			T* new_data = static_cast<T*>(::operator new(sizeof(T) * new_capacity));
			std::memcpy(new_data, data_, sizeof(T) * size_);
			::operator delete(data_);
			data_ = new_data;
			capacity_ = new_capacity;
		}
	}

	void resize_uninitialized(const size_t n) {
		reserve(n);
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

	void push_back(const T value) {
		if (size_ >= capacity_) {
			reserve(size_ + 1);
		}
		data_[size_++] = value;
	}

	template<typename... _Args>
	void emplace_back(_Args&&... __args) {
		if (size_ >= capacity_) {
			reserve(size_ + 1);
		}
		new (&data_[size_++]) T(std::forward<_Args>(__args)...);
	}

	void extend(const dynarray& other) {
		reserve(size_ + other.size_);
		std::memcpy(data_ + size_, other.data_, sizeof(T) * other.size_);
		size_ += other.size_;
	}

private:
	T* data_ = nullptr;
	size_t size_ = 0;
	size_t capacity_ = 0;
};

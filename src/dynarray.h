#pragma once
#include <cstring>
#include <type_traits>

template<typename T>
class dynarray {
public:
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
		if constexpr (!std::is_trivially_destructible_v<T>) {
			for (size_t i = 0; i < size_; ++i) {
				data_[i].~T();
			}
		}
		::operator delete(data_);
	}

	T* data() { return data_; }
	const T* data() const { return data_; }
	size_t size() const { return size_; }
	T& operator[](size_t index) {
		// if (index >= size_) {
			// throw std::out_of_range("Index out of bounds");
		// }
		return data_[index];
	}

private:
	T* data_ = nullptr;
	size_t size_ = 0;
	size_t capacity_ = 0;
};

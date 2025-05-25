#pragma once

#include <chrono>
#include <iostream>

struct Timeit {
	const char* name;
	std::chrono::time_point<std::chrono::high_resolution_clock> start;
	std::chrono::time_point<std::chrono::high_resolution_clock> end {};
	Timeit(const char *name) : name(name), start(std::chrono::high_resolution_clock::now()) {}

	void stop() {
		end = std::chrono::high_resolution_clock::now();
	}

	~Timeit() {
		if (end == std::chrono::time_point<std::chrono::high_resolution_clock>()) {
			end = std::chrono::high_resolution_clock::now();
		}
		static constexpr const char* units[] {"ns", "us", "ms", "s"};
		auto duration = duration_cast<std::chrono::nanoseconds>(end - start).count();
		size_t unit_idx = 0;
		while (duration > 10000 && unit_idx < 3) {
			duration /= 1000;
			unit_idx++;
		}
		std::cout << "Timeit [" << name << "]: " << duration << " " << units[unit_idx] << "\n";
	}
};

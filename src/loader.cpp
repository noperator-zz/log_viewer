#include "loader.h"

#include "parser.h"
#include "search.h"
#include "util.h"

using namespace std::chrono;

struct Match {
	size_t start;
	size_t end;
};

static std::vector<Match> matches;
static int handler (unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags, void *context) {
	printf("Match found: id=%u, from=%llu, to=%llu, flags=%u, context=%p\n", id, from, to, flags, context);
	matches.push_back({from, to});
	return 0; // Continue matching
}

Loader::Loader(File &&file, size_t num_workers)
	: file_(std::move(file)), workers_(num_workers) {
}

Loader::~Loader() {
	workers_.close();
}

std::thread Loader::start(const Event &quit) {
	return std::thread(&Loader::worker, this, std::ref(quit));
}

Loader::Snapshot Loader::snapshot(const std::lock_guard<std::mutex> &lock) const {
	(void)lock;
	return {state_, longest_line_, line_ends_, file_.mapped_data()};
}

void Loader::worker(const Event &quit) {
	{
		Timeit timeit("File Open");
		if (file_.open() != 0) {
			std::cerr << "Failed to open file_\n";
			// TODO better error handling
			return;
		}
	}
	{
		Timeit timeit("File Initial Mapping");
		if (file_.mmap() != 0) {
			std::cerr << "Failed to map file_\n";
			// TODO better error handling
			return;
		}
		timeit.stop();
		std::cout << "File mapped: " << file_.mapped_size() << " B\n";
	}
	while (!quit.wait(0ms)) {
		switch (state_) {
			case State::INIT: {
				load_inital();
				break;
			}
			case State::LOAD_TAIL:
				load_tail();
				(void)quit.wait(100ms);
				break;
			// default:
			// 	assert(false);
		}
	}
}

void Loader::load_inital() {
	size_t total_size = file_.mapped_size();
	std::cout << "Loading " << total_size / 1024 / 1024 << " MiB\n";
	Timeit total_timeit("Load mapped");
	Timeit load_timeit("Load");
	Timeit first_timeit("First chunk");

	size_t chunk_size = 10ULL * 1024 * 1024;
	// size_t chunk_size = 1024;
	size_t num_chunks = (total_size + chunk_size - 1) / chunk_size;
	// size_t num_chunks = 8;
	// size_t chunk_size = total_size / num_chunks;

	std::vector<WorkUnit> units;
	units.reserve(num_chunks);

	size_t i = 0;
	for (i = 0; i < num_chunks; ++i) {
		auto offset = i * chunk_size;
		size_t actual_size = (i == num_chunks - 1) ? (total_size - offset) : chunk_size;
		units.emplace_back(std::vector<size_t>{}, file_.mapped_data() + offset, actual_size, offset);
	}

	// std::this_thread::sleep_for(1s);

	// Load the last chunk first
	i = num_chunks - 1;
	file_.touch_pages(units[i].data, units[i].size);
	workers_.push([this, &units, i](const Event &q) {
		find_newlines_avx2(units[i]);
	});

	// std::this_thread::sleep_for(1s);
	workers_.wait_free(8); // Wait for the last job to finish
	{
		std::lock_guard lock(mtx);
		state_ = State::FIRST_READY;
		line_ends_ = units[i].output;
		longest_line_ = units[i].longest_line;
		mapped_lines_ = units[i].output.size();
	}
	first_timeit.stop();

	for (i = 0; i < num_chunks - 1; ++i) {
		file_.touch_pages(units[i].data, units[i].size);
		workers_.push([this, &units, i](const Event &q) {
			find_newlines_avx2(units[i]);
		});
	}

	// std::this_thread::sleep_for(1s);
	workers_.wait_free(8); // Wait for the last job to finish

	load_timeit.stop();
	Timeit combine_timeit("Combine");

	{
		std::lock_guard lock(mtx);
		state_ = State::LOAD_TAIL;
		mapped_lines_ = 0;
		line_ends_.clear();
		line_ends_.reserve(mapped_lines_);
		// line_ends.push_back(0);
		size_t prev = 0;
		for (const auto& unit : units) {
			if (unit.output.empty()) {
				continue; // Skip empty units
			}
			size_t line_len = unit.output.front() - prev;
			prev = unit.output.back();
			line_len = std::max(line_len, unit.longest_line);
			mapped_lines_ += unit.output.size();
			longest_line_ = std::max(longest_line_, line_len);
			line_ends_.insert(line_ends_.end(), unit.output.begin(), unit.output.end());
		}
		if (!line_ends_.empty() && line_ends_.back() < (total_size - 1)) {
			// NOTE Mapped data is truncated to the last newline. The tailed_data will contain the rest.
			printf("Ignoring %zu bytes at the end of the mapped data\n", total_size - line_ends_.back());
			// // Ensure the last line ends at the end of the file
			// longest_line_ = std::max(longest_line_, total_size - line_ends_.back());
			// line_ends_.push_back(total_size);
			// mapped_lines_++;
		}
		// file_.seek(line_ends_.empty() ? 0 : line_ends_.back());
	}

	combine_timeit.stop();
	total_timeit.stop();

	i = 0;
	for (const auto& unit : units) {
		printf("Unit %3zu (0x%08zx - 0x%08zx): First line 0x%08zx, Last line 0x%08zx, # lines %9zu\n",
			i, unit.offset, unit.offset + unit.size,
			unit.output.empty() ? 0 : unit.output.front(),
			unit.output.empty() ? 0 : unit.output.back(),
			unit.output.size());
		i++;
		fflush(stdout);
	}

	std::cout << "Found " << mapped_lines_ << " newlines\n";
	std::cout << "Longest line: " << longest_line_ << "\n";
	fflush(stdout);
	// assert(mapped_lines_ == 1000);
	// assert(longest_line_ == 4092);
	// assert(mapped_lines_ == 27294137);
	// assert(longest_line_ == 6567);

	{
		Timeit handler_timeit("search");
		search("TkMPPopupHandler", (const char*)file_.mapped_data(), file_.mapped_size(), handler);
	}
	printf("Search found %llu matches\n", matches.size());
}

void Loader::load_tail() {
	auto prev_size = file_.mapped_size();
	{
		Timeit timeit("File remap");
		if (file_.mmap() != 0) {
			std::cerr << "Failed to map file_\n";
			// TODO better error handling
			return;
		}
		timeit.stop();
		std::cout << "File remapped: " << prev_size << " B -> " << file_.mapped_size() << " B\n";
	}

	if (prev_size == file_.mapped_size()) {
		return;
	}
}

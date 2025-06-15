#include <iostream>

#include "file_view.h"

#include <mutex>

#include "parser.h"
#include "search.h"
#include "../shaders/text_shader.h"
#include "util.h"

using namespace glm;
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

std::unique_ptr<FileView> FileView::create(const char *path) {
	return std::unique_ptr<FileView>(new FileView(path));
}

FileView::FileView(const char *path)
	: loader_(File{path}, 8), scroll_h_(true, [this](double p){scroll_h_cb(p);}), scroll_v_(false, [this](double p){scroll_v_cb(p);}) {

	add_child(&scroll_h_);
	add_child(&scroll_v_);
}

FileView::~FileView() {
	quit_.set();
	if (loader_thread_.joinable()) {
		loader_thread_.join();
	}
	TextShader::destroy_buffers(content_buf_);
}

FileView::Loader::Loader(File &&file, size_t num_workers)
	: file_(std::move(file)), workers_(num_workers) {
}

FileView::Loader::~Loader() {
	workers_.close();
}

std::thread FileView::Loader::start(const Event &quit) {
	return std::thread(&Loader::worker, this, std::ref(quit));
}

FileView::Loader::Snapshot FileView::Loader::snapshot(const std::lock_guard<std::mutex> &lock) const {
	(void)lock;
	return {state_, longest_line_, line_ends_, file_.mapped_data()};
}

int FileView::open() {

	TextShader::create_buffers(content_buf_, CONTENT_BUFFER_SIZE);
	TextShader::create_buffers(linenum_buf_, LINENUM_BUFFER_SIZE);

	loader_thread_ = loader_.start(quit_);

	return 0;
}

void FileView::Loader::worker(const Event &quit) {
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

void FileView::Loader::load_inital() {
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

void FileView::Loader::load_tail() {
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

	// size_t total = file_.read_tail();
	// std::cout << "Loading tail " << total << " bytes\n";
	// auto offset = line_ends_.empty() ? 0 : line_ends_[mapped_lines_ - 1];
	// size_t parsed_tailed_bytes = line_ends_.empty() ? 0 : line_ends_.back() - line_ends_[mapped_lines_ - 1];
	// if (file_.tailed_size() > parsed_tailed_bytes) {
	// 	WorkUnit unit {
	// 		std::vector<size_t>{},
	// 		file_.tailed_data() + parsed_tailed_bytes,
	// 		file_.tailed_size() - parsed_tailed_bytes,
	// 		parsed_tailed_bytes + offset
	// 	};
	// 	find_newlines_avx2(unit);
	// 	{
	// 		std::lock_guard lock(mtx);
	// 		size_t num_lines = unit.output.size();
	// 		if (num_lines > 0) {
	// 			line_ends_.insert(line_ends_.end(), unit.output.begin(), unit.output.end());
	// 			printf("Tailed data: First line 0x%08zx, Last line 0x%08zx, # lines %9zu\n",
	// 				unit.output.front(), unit.output.back(), num_lines);
	// 		}
	// 	}
	// }
}

void FileView::on_resize() {
	scroll_h_.resize({pos().x, pos().y + size().y - 30}, {size().x, 30});
	scroll_v_.resize({pos().x + size().x - 30, pos().y}, {30, size().y});
	update_scrollbar();
}

void FileView::update_scrollbar() {
	scroll_h_.set(scroll_.x, size().x, longest_line_ * TextShader::font().size.x);
	scroll_v_.set(scroll_.y, size().y, (num_lines_ - 1) * TextShader::font().size.y);
}

void FileView::scroll_h_cb(double percent) {
	scroll_.x = (int)(percent * longest_line_ * TextShader::font().size.x);
	scroll_.x = std::max(scroll_.x, 0);
	scroll_.x = std::min(scroll_.x, (int)longest_line_ * TextShader::font().size.x);
	update_scrollbar();
}

void FileView::scroll_v_cb(double percent) {
	scroll_.y = (int)(percent * (num_lines_ - 1) * TextShader::font().size.y);
	scroll_.y = std::max(scroll_.y, 0);
	scroll_.y = std::min(scroll_.y, (int)(num_lines_ - 1) * TextShader::font().size.y);
	update_scrollbar();
}

void FileView::scroll(ivec2 scroll) {
	// scroll *= TextShader::font().size;
	scroll *= -3;

	scroll_.x += std::max(scroll.x, -scroll_.x);
	scroll_.y += std::max(scroll.y, -scroll_.y);

	update_scrollbar();
}

static size_t linenum_len(size_t line) {
	return line == 0 ? 1 : (size_t)log10(line) + 1;
}

size_t FileView::get_line_start(Loader::State state, size_t line_idx, const std::vector<size_t> &line_ends) {
	if (line_idx == 0) {
		if (state == Loader::State::FIRST_READY) {
			return line_ends[line_idx];
		}
		return 0;
	}
	return line_ends[line_idx - 1];
}

void FileView::really_update_buffers(int start, int end, const Loader::Snapshot &snapshot) {

	// Timeit update_timeit("Update content buffer");

	// first (inclusive) and last (exclusive) lines to buffer
	buf_lines_ = {start, end};
	assert(buf_lines_.y > 0);
	assert(!snapshot.line_ends.empty());

	// TODO The lines on screen have already been parsed. Cache and reuse them instead of re-parsing

	const auto first_line_start = get_line_start(snapshot.state, buf_lines_.x, snapshot.line_ends);

	// TODO detect if content won't fit in the buffer
	// TODO If there is an extremely long line, only buffer the visible portion of the line
	// TODO Handle 0 mapped lines, and other cases
	// TODO Use glMapBufferRange
	glBindBuffer(GL_ARRAY_BUFFER, content_buf_.vbo_text);
	size_t num_chars = 0;
	num_chars = snapshot.line_ends[buf_lines_.y-1] - first_line_start;
	glBufferSubData(GL_ARRAY_BUFFER, 0, num_chars, snapshot.data + first_line_start);

	auto content_styles = std::make_unique<TextShader::CharStyle[]>(num_chars);
	size_t c = 0;
	size_t current_char = first_line_start;
	size_t prev_end = first_line_start;

	auto next_match = matches.begin();

	for (uint line_idx = buf_lines_.x; line_idx < buf_lines_.y; line_idx++) {
		while (next_match != matches.end() && current_char > next_match->end) {
			// Skip matches that are before the current character
			next_match++;
		}


		size_t line_len = snapshot.line_ends[line_idx] - prev_end;
		prev_end = snapshot.line_ends[line_idx];
		for (size_t i = 0; i < line_len; i++) {
			bool is_match = next_match != matches.end() && current_char >= next_match->start && current_char < next_match->end;
			content_styles[c] = {
				{},
				uvec2{i, line_idx},
				vec4{is_match ? 200 : 100, 200, 200, 255},
				vec4{100, 100, 100, 100}
			};
			c++;
			current_char++;
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, content_buf_.vbo_style);
	glBufferSubData(GL_ARRAY_BUFFER, 0, num_chars * sizeof(TextShader::CharStyle), content_styles.get());

	const auto num_lines = buf_lines_.y - buf_lines_.x;
	// max number of characters for all line numbers
	const auto total_linenum_chars = linenum_chars_ * num_lines;

	auto linenum_text = std::make_unique<uint8_t[]>(total_linenum_chars + 1); // +1 for the null terminator added by sprintf
	auto linenum_styles = std::make_unique<TextShader::CharStyle[]>(total_linenum_chars);

	c = 0;

	const std::string fmt = "%" + std::to_string(linenum_chars_) + "zu";
	for (uint line_idx = buf_lines_.x; line_idx < buf_lines_.y; line_idx++) {
		// std::string linenum_str = std::to_string(line_idx + 1);
		// linenum_text.insert(linenum_text.end(), linenum_str.begin(), linenum_str.end());
		size_t line_len;
		if (snapshot.state == Loader::State::FIRST_READY) {
			linenum_text[c] = '?';
			line_len = 1;
		} else {
			line_len = sprintf((char*)&linenum_text[c], fmt.c_str(), line_idx + 1);
		}

		for (size_t i = 0; i < line_len; i++) {
			linenum_styles[c] = {
				{},
				uvec2{i, line_idx},
				vec4{200, 200, 200, 255},
				vec4{100, 100, 100, 100}
			};
			c++;
		}
	}
	glBindBuffer(GL_ARRAY_BUFFER, linenum_buf_.vbo_text);
	glBufferSubData(GL_ARRAY_BUFFER, 0, total_linenum_chars, linenum_text.get());

	glBindBuffer(GL_ARRAY_BUFFER, linenum_buf_.vbo_style);
	glBufferSubData(GL_ARRAY_BUFFER, 0, total_linenum_chars * sizeof(TextShader::CharStyle), linenum_styles.get());

	// update_timeit.stop();
	// printf("Content buffer size: %zu\n", num_chars * (sizeof(TextShader::CharStyle) + sizeof(uint8_t)));
	// printf("Linenum buffer size: %zu\n", total_linenum_chars * (sizeof(TextShader::CharStyle) + sizeof(uint8_t)));
}

void FileView::update_buffers(uvec2 &content_render_range, uvec2 &linenum_render_range, const Loader::Snapshot &snapshot) {
	content_render_range = {};
	linenum_render_range = {};

	if (snapshot.state == Loader::State::INIT) {
		return;
	}

	if (!num_lines_) {
		return;
	}

	// first (inclusive) and last (exclusive) lines which would be visible on the screen
	ivec2 visible_lines = ivec2{0, 1} + ivec2{scroll_.y, scroll_.y + size().y} / TextShader::font().size.y;

	// Clamp to the total number of lines available
	visible_lines.x = std::min(visible_lines.x, (int)num_lines_ - 1);
	visible_lines.y = std::min(visible_lines.y, (int)num_lines_);

	// Check if the visible lines are already in the buffer
	if (visible_lines.x < buf_lines_.x || visible_lines.y > buf_lines_.y) {
		really_update_buffers(
			// first (inclusive) and last (exclusive) lines to buffer
			std::max(0LL, visible_lines.x - OVERSCAN_LINES),
			std::min((ssize_t)num_lines_, visible_lines.y + OVERSCAN_LINES),
			snapshot
		);
	}

	content_render_range = u64vec2{get_line_start(snapshot.state, visible_lines.x, snapshot.line_ends), snapshot.line_ends[visible_lines.y - 1]} - get_line_start(snapshot.state, buf_lines_.x, snapshot.line_ends);
	linenum_render_range = (uvec2{visible_lines.x, visible_lines.y} - (uint)buf_lines_.x) * (uint)linenum_chars_;
}

void FileView::draw_lines(const uvec2 render_range) const {
	glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, render_range.y - render_range.x, render_range.x);
}

void FileView::draw_linenums(const uvec2 render_range) const {
	TextShader::use(linenum_buf_);
	TextShader::update_uniforms();
	draw_lines(render_range);

	TextShader::globals.is_foreground = true;
	TextShader::update_uniforms();
	draw_lines(render_range);
}

void FileView::draw_content(const uvec2 render_range) const {
	TextShader::use(content_buf_);
	TextShader::update_uniforms();
	draw_lines(render_range);

	TextShader::globals.is_foreground = true;
	TextShader::update_uniforms();
	draw_lines(render_range);
}

void FileView::draw() {
	uvec2 content_render_range;
	uvec2 linenum_render_range;
	{
		// TODO release this lock sooner by making copies of the data we need
		std::lock_guard lock(loader_.mtx);

		const auto snapshot = loader_.snapshot(lock);
		longest_line_ = snapshot.longest_line;

		auto prev_num_lines = num_lines_;
		num_lines_ = snapshot.line_ends.size();

		if (state_ == Loader::State::INIT && snapshot.state != Loader::State::INIT) {
			// jump to the end of the file
			scroll_.y = num_lines_ * TextShader::font().size.y - size().y;
		} else if (state_ == Loader::State::FIRST_READY && snapshot.state != Loader::State::FIRST_READY) {
			// Re-sync the scroll position after line_ends was updated
			scroll_.y += (num_lines_ - prev_num_lines) * TextShader::font().size.y;
		}

		state_ = snapshot.state;

		if (snapshot.state == Loader::State::FIRST_READY) {
			linenum_chars_ = 1;
		} else {
			linenum_chars_ = linenum_len(num_lines_);
		}
		update_buffers(content_render_range, linenum_render_range, snapshot);
	}

	// std::cout << content_render_range.x << " " << content_render_range.y << "\n";

	// TextShader::globals.frame_offset_px = pos();
	TextShader::globals.scroll_offset_px = scroll_;
	TextShader::globals.is_foreground = false;

	// Timeit draw("Draw");

	TextShader::globals.set_viewport(pos() + ivec2{-(linenum_chars_ * TextShader::font().size.x + 20), 0}, size());
	draw_content(content_render_range);

	TextShader::globals.set_viewport(pos(), size());
	draw_linenums(linenum_render_range);

	scroll_h_.draw();
	scroll_v_.draw();

	// draw.stop();
}

#include <iostream>

#include "file_view.h"

#include <mutex>

#include "parser.h"
#include "../shaders/text_shader.h"
#include "util.h"

using namespace glm;
using namespace std::chrono;

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

FileView::Loader::State FileView::Loader::get_state(const std::lock_guard<std::mutex> &lock) {
	(void)lock;
	return state_;
}

size_t FileView::Loader::get_mapped_lines(const std::lock_guard<std::mutex> &lock) const {
	(void)lock;
	return mapped_lines_;
}

size_t FileView::Loader::get_longest_line(const std::lock_guard<std::mutex> &lock) const {
	(void)lock;
	return longest_line_;
}

const std::vector<size_t> &FileView::Loader::get_line_ends(const std::lock_guard<std::mutex> &lock) const {
	(void)lock;
	return line_ends_;
}

const uint8_t *FileView::Loader::get_mapped_data() const {
	return file_.mapped_data();
}

const uint8_t *FileView::Loader::get_tailed_data() const {
	return file_.tailed_data();
}

int FileView::open() {

	TextShader::create_buffers(content_buf_, CONTENT_BUFFER_SIZE);
	TextShader::create_buffers(linenum_buf_, LINENUM_BUFFER_SIZE);

	loader_thread_ = loader_.start(quit_);

	return 0;
}

void FileView::Loader::worker(const Event &quit) {
	{
		Timeit file_open_timeit("File Open");
		if (file_.open() != 0) {
			std::cerr << "Failed to open file_\n";
			// TODO better error handling
			return;
		}
	}
	while (!quit.wait(0ms)) {
		switch (state_) {
			case State::INIT: {
				load_mapped();
				break;
			}
			case State::LOAD_TAIL:
				load_tailed();
				(void)quit.wait(1s);
				break;
			// default:
			// 	assert(false);
		}
	}
}

void FileView::Loader::load_mapped() {
	size_t total_size = file_.mapped_size();
	std::cout << "Loading " << total_size / 1024 / 1024 << " MiB\n";
	Timeit total_timeit("Load mapped");
	Timeit load_timeit("Load");
	Timeit first_timeit("First chunk");

	size_t chunk_size = 1ULL * 1024 * 1024;
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
		for (i = 0; i < num_chunks; ++i) {
			mapped_lines_ += units[i].output.size();
			longest_line_ = std::max(longest_line_, units[i].longest_line);
		}

		line_ends_.clear();
		line_ends_.reserve(mapped_lines_);
		// line_ends.push_back(0);
		for (const auto& unit : units) {
			line_ends_.insert(line_ends_.end(), unit.output.begin(), unit.output.end());
		}
	}

	combine_timeit.stop();
	total_timeit.stop();
	std::cout << "Found " << mapped_lines_ << " newlines\n";
	std::cout << "Longest line: " << longest_line_ << "\n";
	// assert(num_mmapped_lines_ == 10001);
	assert(mapped_lines_ == 27294137);
	assert(longest_line_ == 6567);
}

void FileView::Loader::load_tailed() {
	// std::cout << "Loading tailed data\n";
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

uvec2 FileView::update_buffers(const Loader::State state, const size_t mapped_lines, const std::vector<size_t> &line_ends,
	const uint8_t *mapped_data, const uint8_t *tailed_data) {
	if (state == Loader::State::INIT) {
		return {};
	}

	if (!num_lines_) {
		return {};
	}

	// first (inclusive) and last (exclusive) lines which would be visible on the screen
	ivec2 visible_lines = ivec2{0, 1} + ivec2{scroll_.y, scroll_.y + size().y} / TextShader::font().size.y;

	// Clamp to the total number of lines available
	visible_lines.x = std::min(visible_lines.x, (int)num_lines_ - 1);
	visible_lines.y = std::min(visible_lines.y, (int)num_lines_);

	// Check if the visible lines are already in the buffer
	if (visible_lines.x >= buf_lines_.x && visible_lines.y <= buf_lines_.y) {
		// Already in the buffer
		return u64vec2{get_line_start(state, visible_lines.x, line_ends), line_ends[visible_lines.y - 1]} - get_line_start(state, buf_lines_.x, line_ends);
	}

	// Timeit update_timeit("Update content buffer");

	// first (inclusive) and last (exclusive) lines to buffer
	buf_lines_.x = std::max(0LL, visible_lines.x - OVERSCAN_LINES);
	buf_lines_.y = std::min((ssize_t)num_lines_, visible_lines.y + OVERSCAN_LINES);
	assert(buf_lines_.y > 0);
	assert(mapped_lines > 0);

	// TODO The lines on screen have already been parsed. Cache and reuse them instead of re-parsing

	const auto first_line_start = get_line_start(state, buf_lines_.x, line_ends);

	// TODO detect if content won't fit in the buffer
	// TODO If there is an extremely long line, only buffer the visible portion of the line
	// TODO Handle 0 mapped lines, and other cases
	// TODO Use glMapBufferRange
	glBindBuffer(GL_ARRAY_BUFFER, content_buf_.vbo_text);
	const uint8_t *ptr;
	size_t num_chars = 0;
	if (buf_lines_.y <= mapped_lines) {
		ptr = mapped_data;
		num_chars = line_ends[buf_lines_.y-1] - first_line_start;
		glBufferSubData(GL_ARRAY_BUFFER, 0, num_chars, ptr + first_line_start);
	} else if (buf_lines_.x >= mapped_lines) {
		ptr = tailed_data;
		num_chars = line_ends[buf_lines_.y-1] - first_line_start;
		const auto offset = line_ends[mapped_lines - 1];
		glBufferSubData(GL_ARRAY_BUFFER, 0, num_chars, ptr + first_line_start - offset);
	} else {
		assert(buf_lines_.x < mapped_lines && buf_lines_.y > mapped_lines);
		ptr = mapped_data;
		num_chars = line_ends[mapped_lines-1] - first_line_start;
		glBufferSubData(GL_ARRAY_BUFFER, 0, num_chars, ptr + first_line_start);

		ptr = tailed_data;
		size_t num_chars2 = line_ends[buf_lines_.y-1] - line_ends[mapped_lines-1];
		glBufferSubData(GL_ARRAY_BUFFER, num_chars, num_chars2, ptr);
		num_chars += num_chars2;
	}

	auto content_styles = std::make_unique<TextShader::CharStyle[]>(num_chars);
	size_t c = 0;
	size_t prev_end = first_line_start;
	for (uint line_idx = buf_lines_.x; line_idx < buf_lines_.y; line_idx++) {
		size_t line_len = line_ends[line_idx] - prev_end;
		prev_end = line_ends[line_idx];
		for (size_t i = 0; i < line_len; i++) {
			content_styles[c] = {
				{},
				uvec2{i, line_idx},
				vec4{200, 200, 200, 255},
				vec4{100, 100, 100, 100}
			};
			c++;
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
		if (state == Loader::State::FIRST_READY) {
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

	return u64vec2{get_line_start(state, visible_lines.x, line_ends), line_ends[visible_lines.y - 1]} - get_line_start(state, buf_lines_.x, line_ends);
}

void FileView::draw_lines(const uvec2 render_size) const {
	glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, render_size.y - render_size.x, render_size.x);
}

void FileView::draw_linenums(const uvec2 render_size) const {
	TextShader::use(linenum_buf_);
	TextShader::update_uniforms();
	draw_lines(render_size);

	TextShader::globals.is_foreground = true;
	TextShader::update_uniforms();
	draw_lines(render_size);
}

void FileView::draw_content(const uvec2 render_size) const {
	TextShader::use(content_buf_);
	TextShader::update_uniforms();
	draw_lines(render_size);

	TextShader::globals.is_foreground = true;
	TextShader::update_uniforms();
	draw_lines(render_size);
}

void FileView::draw() {
	uvec2 render_size;
	{
		// TODO release this lock sooner by making copies of the data we need
		std::lock_guard lock(loader_.mtx);

		const auto state = loader_.get_state(lock);
		const auto mapped_lines = loader_.get_mapped_lines(lock);
		longest_line_ = loader_.get_longest_line(lock);
		const auto &line_ends = loader_.get_line_ends(lock);
		const auto mapped_data = loader_.get_mapped_data();
		const auto tailed_data = loader_.get_tailed_data();

		auto prev_num_lines = num_lines_;
		num_lines_ = line_ends.size();

		if (state_ == Loader::State::INIT && state != Loader::State::INIT) {
			// jump to the end of the file
			scroll_.y = num_lines_ * TextShader::font().size.y - size().y;
		} else if (state_ == Loader::State::FIRST_READY && state != Loader::State::FIRST_READY) {
			// Re-sync the scroll position after line_ends was updated
			scroll_.y += (num_lines_ - prev_num_lines) * TextShader::font().size.y;
		}

		state_ = state;

		if (state == Loader::State::FIRST_READY) {
			linenum_chars_ = 1;
		} else {
			linenum_chars_ = linenum_len(mapped_lines);
		}
		render_size = update_buffers(state, mapped_lines, line_ends, mapped_data, tailed_data);
	}

	std::cout << render_size.x << " " << render_size.y << "\n";

	// TextShader::globals.frame_offset_px = pos();
	TextShader::globals.scroll_offset_px = scroll_;
	TextShader::globals.is_foreground = false;

	// Timeit draw("Draw");

	TextShader::globals.set_viewport(pos() + ivec2{-(linenum_chars_ * TextShader::font().size.x + 20), 0}, size());
	draw_content(render_size);

	TextShader::globals.set_viewport(pos(), size());
	draw_linenums(render_size);

	scroll_h_.draw();
	scroll_v_.draw();

	// draw.stop();
}

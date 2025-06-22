#include <iostream>

#include "file_view.h"

#include <mutex>

#include "parser.h"
#include "search.h"
#include "../shaders/text_shader.h"
#include "util.h"

using namespace glm;
using namespace std::chrono;

std::unique_ptr<FileView> FileView::create(const char *path) {
	return std::unique_ptr<FileView>(new FileView(path));
}

FileView::FileView(const char *path)
	: Widget("FV"), loader_(File{path}, 8) {

	add_child(linenum_view_);
	add_child(content_view_);

	find_views_.emplace_back(std::make_unique<FindView>([this](const FindView &find_view) {}));
	add_child(*find_views_.back().get());
}

FileView::~FileView() {
	quit_.set();
	if (loader_thread_.joinable()) {
		loader_thread_.join();
	}
	TextShader::destroy_buffers(content_view_.buf_);
	TextShader::destroy_buffers(linenum_view_.buf_);
}

int FileView::open() {

	TextShader::create_buffers(content_view_.buf_, CONTENT_BUFFER_SIZE);
	TextShader::create_buffers(linenum_view_.buf_, LINENUM_BUFFER_SIZE);

	loader_thread_ = loader_.start(quit_);

	return 0;
}

void FileView::on_resize() {
	auto linenum_width = linenum_chars_ * TextShader::font().size.x + 20;
	linenum_view_.resize(pos(), {linenum_width, size().y});
	content_view_.resize({pos().x + linenum_width, pos().y}, {size().x - linenum_width, size().y});
	auto p = content_view_.pos();
	for (auto &find_view : find_views_) {
		find_view->resize(p, {content_view_.size().x - content_view_.scroll_v_.size().x, 30});
		p.y += 30;
	}
}

void FileView::scroll_h_cb(double percent) {
	scroll_.x = (int)(percent * longest_line_ * TextShader::font().size.x);
	scroll_.x = std::max(scroll_.x, 0);
	scroll_.x = std::min(scroll_.x, (int)longest_line_ * TextShader::font().size.x);
	content_view_.update_scrollbar();
}

void FileView::scroll_v_cb(double percent) {
	scroll_.y = (int)(percent * num_lines() * TextShader::font().size.y);
	scroll_.y = std::max(scroll_.y, 0);
	scroll_.y = std::min(scroll_.y, (int)num_lines() * TextShader::font().size.y);
	content_view_.update_scrollbar();
}

void FileView::scroll(ivec2 scroll) {
	scroll *= TextShader::font().size;
	scroll *= -3;

	scroll_.x += std::max(scroll.x, -scroll_.x);
	scroll_.y += std::max(scroll.y, -scroll_.y);

	content_view_.update_scrollbar();
}

static size_t linenum_len(size_t line) {
	return line == 0 ? 1 : (size_t)log10(line) + 1;
}

size_t FileView::get_line_len(size_t line_idx) const {
	assert(line_idx < line_starts_.size() - 1);
	return line_starts_[line_idx + 1] - line_starts_[line_idx];
}

size_t FileView::num_lines() const {
	return line_starts_.empty() ? 0 : line_starts_.size() - 1;
}

// NOTE Terminology:
//  - abs_pos:         Pixel coordinate (x, y) relative to the window
//  - view_pos:        Pixel coordinate (x, y) relative to the widget position
//  - abs_char_loc:    Character coordinate (col, row/line) relative to the start of the file
//  - buf_char_loc:    Character coordinate (col, row/line) relative to the start of the GPU buffer
//  - abs_char_idx:    Character index relative to the start of the file
//  - buf_char_idx:    Character index relative to the start of the GPU buffer
//  - buf:             The subset of lines currently loaded in the GPU buffer, which is larger than the visible range by ~2 * OVERSCAN_LINES

// NOTE Trivial Conversions:
//  abs_pos -> view_pos: rel_pos(abs_pos)
//  -
//  abs_char_loc -> abs_char_idx: line_starts[abs_loc.y] + abs_loc.x
//  abs_char_loc -> buf_char_loc: abs_loc - {buf_lines_.x, 0}
//  -
//  abs_char_idx -> buf_char_idx: abs_idx - line_starts[buf_lines_.x]
//  buf_char_loc -> buf_char_idx: abs_char_loc(buf_loc) - line_start[buf_lines_.x]
//  -

size_t FileView::abs_char_loc_to_abs_char_idx(const ivec2 &abs_loc) const {
	return line_starts_[abs_loc.y] + abs_loc.x;
}

size_t FileView::abs_char_idx_to_buf_char_idx(size_t abs_idx) const {
	if (abs_idx < line_starts_[buf_lines_.x]) {
		return 0; // If the index is before the start of the buffer, return 0
	}
	if (abs_idx > line_starts_[buf_lines_.y]) {
		abs_idx = line_starts_[buf_lines_.y];
	}
	return abs_idx - line_starts_[buf_lines_.x];
}

size_t FileView::abs_char_loc_to_buf_char_idx(const ivec2 &abs_loc) const {
	return abs_char_idx_to_buf_char_idx(abs_char_loc_to_abs_char_idx(abs_loc));
}

void FileView::really_update_buffers(int start, int end, const uint8_t *data) {
	// Timeit update_timeit("Update content buffer");

	// first (inclusive) and last (exclusive) lines to buffer
	buf_lines_ = {start, end};
	assert(buf_lines_.y > 0);
	assert(num_lines() > 0);

	// TODO The lines on screen have already been parsed. Cache and reuse them instead of re-parsing

	const auto first_line_start = line_starts_[buf_lines_.x];

	// TODO detect if content won't fit in the buffer
	// TODO If there is an extremely long line, only buffer the visible portion of the line
	// TODO Handle 0 mapped lines, and other cases
	// TODO Use glMapBufferRange
	glBindBuffer(GL_ARRAY_BUFFER, content_view_.buf_.vbo_text);
	size_t num_chars = 0;
	num_chars = line_starts_[buf_lines_.y] - first_line_start;
	glBufferSubData(GL_ARRAY_BUFFER, 0, num_chars, data + first_line_start);

	content_view_.base_styles_.resize_uninitialized(num_chars);
	content_view_.mod_styles_.resize_uninitialized(num_chars);
	size_t c = 0;
	size_t current_char = first_line_start;

	// auto next_match = matches.begin();

	for (uint line_idx = buf_lines_.x; line_idx < buf_lines_.y; line_idx++) {
		// while (next_match != matches.end() && current_char > next_match->end) {
		// 	// Skip matches that are before the current character
		// 	next_match++;
		// }

		size_t line_len = get_line_len(line_idx);
		for (size_t i = 0; i < line_len; i++) {
			bool is_match = false;//next_match != matches.end() && current_char >= next_match->start && current_char < next_match->end;
			uint8_t r = is_match ? 100 : 200;
			uint8_t g = 200;
			uint8_t b = 200;
			content_view_.base_styles_[c] = {
				{},
				uvec2{i, line_idx},
				vec4{r, g, b, 255},
				vec4{}
				// vec4{100, 0, 0, 100}
			};
			c++;
			current_char++;
		}
	}

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
		size_t line_len = sprintf((char*)&linenum_text[c], fmt.c_str(), line_idx + 1);

		for (size_t i = 0; i < line_len; i++) {
			linenum_styles[c] = {
				{},
				uvec2{i, line_idx},
				vec4{0xA0, 0xA0, 0xA0, 255},
				vec4{}
			};
			c++;
		}
	}
	glBindBuffer(GL_ARRAY_BUFFER, linenum_view_.buf_.vbo_text);
	glBufferSubData(GL_ARRAY_BUFFER, 0, total_linenum_chars, linenum_text.get());

	glBindBuffer(GL_ARRAY_BUFFER, linenum_view_.buf_.vbo_style);
	glBufferSubData(GL_ARRAY_BUFFER, 0, total_linenum_chars * sizeof(TextShader::CharStyle), linenum_styles.get());

	// update_timeit.stop();
	// printf("Content buffer size: %zu\n", num_chars * (sizeof(TextShader::CharStyle) + sizeof(uint8_t)));
	// printf("Linenum buffer size: %zu\n", total_linenum_chars * (sizeof(TextShader::CharStyle) + sizeof(uint8_t)));
}

void FileView::update_buffers(uvec2 &content_render_range, uvec2 &linenum_render_range, const uint8_t *data) {
	content_render_range = {};
	linenum_render_range = {};

	if (num_lines() == 0) {
		return;
	}

	// first (inclusive) and last (exclusive) lines which would be visible on the screen
	ivec2 visible_lines = ivec2{0, 1} + ivec2{scroll_.y, scroll_.y + size().y} / TextShader::font().size.y;

	// Clamp to the total number of lines available
	visible_lines.x = std::min(visible_lines.x, (int)num_lines() - 1);
	visible_lines.y = std::min(visible_lines.y, (int)num_lines());

	// Check if the visible lines are already in the buffer
	if (visible_lines.x < buf_lines_.x || visible_lines.y > buf_lines_.y) {
		really_update_buffers(
			// first (inclusive) and last (exclusive) lines to buffer
			std::max(0LL, visible_lines.x - OVERSCAN_LINES),
			std::min((ssize_t)num_lines(), visible_lines.y + OVERSCAN_LINES),
			data
		);
	}

	auto buf_start_char = line_starts_[buf_lines_.x];
	content_render_range = u64vec2{
		line_starts_[visible_lines.x],
		line_starts_[visible_lines.y]
	} - buf_start_char;

	linenum_render_range = (uvec2{visible_lines.x, visible_lines.y} - (uint)buf_lines_.x) * (uint)linenum_chars_;
}

void FileView::draw() {
	const uint8_t *data;
	auto prev_linenum_chars = linenum_chars_;
	auto prev_num_lines = num_lines();

	{
		std::lock_guard lock(loader_.mtx);
		loader_.update(line_starts_, longest_line_, data);

		if (prev_num_lines == 0 && num_lines() > 0) {
			// jump to the end of the file
			scroll_.y = num_lines() * TextShader::font().size.y - size().y;
		}

		linenum_chars_ = linenum_len(num_lines());

		update_buffers(content_view_.render_range_, linenum_view_.render_range_, data);
	}
	if (linenum_chars_ != prev_linenum_chars) {
		on_resize();
	}
	// TODO this is only needed if longest_line_ or num_lines_ changed
	content_view_.update_scrollbar();

	// std::cout << content_render_range.x << " " << content_render_range.y << "\n";

	// TextShader::globals.frame_offset_px = pos();

	// Timeit draw("Draw");
	//
	content_view_.draw();
	linenum_view_.draw();

	for (auto &find_view : find_views_) {
		find_view->draw();
	}

	// draw.stop();
}

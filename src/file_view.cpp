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
	: loader_(File{path}, 8) {

	add_child(&linenum_view_);
	add_child(&content_view_);
	add_child(&scroll_h_);
	add_child(&scroll_v_);
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
	scroll_h_.resize({pos().x + linenum_width, pos().y + size().y - SCROLL_W}, {size().x - linenum_width - SCROLL_W, SCROLL_W});
	scroll_v_.resize({pos().x + size().x - SCROLL_W, pos().y}, {SCROLL_W, size().y});
	update_scrollbar();
}

void FileView::update_scrollbar() {
	scroll_h_.set(scroll_.x, size().x, longest_line_ * TextShader::font().size.x);
	scroll_v_.set(scroll_.y, size().y, (line_ends_.size() - 1) * TextShader::font().size.y);
}

void FileView::scroll_h_cb(double percent) {
	scroll_.x = (int)(percent * longest_line_ * TextShader::font().size.x);
	scroll_.x = std::max(scroll_.x, 0);
	scroll_.x = std::min(scroll_.x, (int)longest_line_ * TextShader::font().size.x);
	update_scrollbar();
}

void FileView::scroll_v_cb(double percent) {
	scroll_.y = (int)(percent * (line_ends_.size() - 1) * TextShader::font().size.y);
	scroll_.y = std::max(scroll_.y, 0);
	scroll_.y = std::min(scroll_.y, (int)(line_ends_.size() - 1) * TextShader::font().size.y);
	update_scrollbar();
}

void FileView::scroll(ivec2 scroll) {
	scroll *= TextShader::font().size;
	scroll *= -3;

	scroll_.x += std::max(scroll.x, -scroll_.x);
	scroll_.y += std::max(scroll.y, -scroll_.y);

	update_scrollbar();
}

static size_t linenum_len(size_t line) {
	return line == 0 ? 1 : (size_t)log10(line) + 1;
}

size_t FileView::get_line_start(size_t line_idx, const std::vector<size_t> &line_ends) {
	if (line_idx == 0) {
		return 0;
	}
	return line_ends[line_idx - 1];
}

size_t FileView::get_line_len(size_t line_idx, const std::vector<size_t> &line_ends) {
	return line_ends[line_idx] - get_line_start(line_idx, line_ends);
}

void FileView::really_update_buffers(int start, int end, const uint8_t *data) {
	// Timeit update_timeit("Update content buffer");

	// first (inclusive) and last (exclusive) lines to buffer
	buf_lines_ = {start, end};
	assert(buf_lines_.y > 0);
	assert(!line_ends_.empty());

	// TODO The lines on screen have already been parsed. Cache and reuse them instead of re-parsing

	const auto first_line_start = get_line_start(buf_lines_.x, line_ends_);

	// TODO detect if content won't fit in the buffer
	// TODO If there is an extremely long line, only buffer the visible portion of the line
	// TODO Handle 0 mapped lines, and other cases
	// TODO Use glMapBufferRange
	glBindBuffer(GL_ARRAY_BUFFER, content_view_.buf_.vbo_text);
	size_t num_chars = 0;
	num_chars = line_ends_[buf_lines_.y-1] - first_line_start;
	glBufferSubData(GL_ARRAY_BUFFER, 0, num_chars, data + first_line_start);

	auto content_styles = std::make_unique<TextShader::CharStyle[]>(num_chars);
	size_t c = 0;
	size_t current_char = first_line_start;
	size_t prev_end = first_line_start;

	// auto next_match = matches.begin();

	for (uint line_idx = buf_lines_.x; line_idx < buf_lines_.y; line_idx++) {
		// while (next_match != matches.end() && current_char > next_match->end) {
		// 	// Skip matches that are before the current character
		// 	next_match++;
		// }

		size_t line_len = line_ends_[line_idx] - prev_end;
		prev_end = line_ends_[line_idx];
		for (size_t i = 0; i < line_len; i++) {
			bool is_match = false;//next_match != matches.end() && current_char >= next_match->start && current_char < next_match->end;
			uint8_t r = is_match ? 200 : 100;
			uint8_t g = 200;
			uint8_t b = 200;
			content_styles[c] = {
				{},
				uvec2{i, line_idx},
				vec4{r, g, b, 255},
				vec4{100, 100, 100, 100}
			};
			c++;
			current_char++;
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, content_view_.buf_.vbo_style);
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
		size_t line_len = sprintf((char*)&linenum_text[c], fmt.c_str(), line_idx + 1);

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
	glBindBuffer(GL_ARRAY_BUFFER, linenum_view_.buf_.vbo_text);
	glBufferSubData(GL_ARRAY_BUFFER, 0, total_linenum_chars, linenum_text.get());

	glBindBuffer(GL_ARRAY_BUFFER, linenum_view_.buf_.vbo_style);
	glBufferSubData(GL_ARRAY_BUFFER, 0, total_linenum_chars * sizeof(TextShader::CharStyle), linenum_styles.get());

	// update_timeit.stop();
	// printf("Content buffer size: %zu\n", num_chars * (sizeof(TextShader::CharStyle) + sizeof(uint8_t)));
	// printf("Linenum buffer size: %zu\n", total_linenum_chars * (sizeof(TextShader::CharStyle) + sizeof(uint8_t)));
}

void FileView::update_buffers(uvec2 &content_render_range, uvec2 &linenum_render_range, const uint8_t *data) {
	auto num_lines_ = line_ends_.size();
	content_render_range = {};
	linenum_render_range = {};

	if (line_ends_.empty()) {
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
			data
		);
	}

	auto buf_start_char = get_line_start(buf_lines_.x, line_ends_);
	content_render_range = u64vec2{
		get_line_start(visible_lines.x, line_ends_),
		line_ends_[visible_lines.y - 1]
	} - buf_start_char;

	linenum_render_range = (uvec2{visible_lines.x, visible_lines.y} - (uint)buf_lines_.x) * (uint)linenum_chars_;
}

void FileView::draw_lines(const uvec2 render_range) const {
	glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, render_range.y - render_range.x, render_range.x);
}

void FileView::draw() {
	const uint8_t *data;
	auto prev_linenum_chars = linenum_chars_;
	auto prev_num_lines = line_ends_.size();

	{
		std::lock_guard lock(loader_.mtx);
		loader_.update(line_ends_, longest_line_, data);

		if (prev_num_lines == 0 && !line_ends_.empty()) {
			// jump to the end of the file
			scroll_.y = line_ends_.size() * TextShader::font().size.y - size().y;
		}

		linenum_chars_ = linenum_len(line_ends_.size());

		update_buffers(content_view_.render_range_, linenum_view_.render_range_, data);
	}
	if (linenum_chars_ != prev_linenum_chars) {
		on_resize();
	}
	// TODO this is only needed if longest_line_ or num_lines_ changed
	update_scrollbar();

	// std::cout << content_render_range.x << " " << content_render_range.y << "\n";

	// TextShader::globals.frame_offset_px = pos();
	TextShader::globals.scroll_offset_px = scroll_;
	TextShader::globals.is_foreground = false;

	// Timeit draw("Draw");
	//
	content_view_.draw();
	linenum_view_.draw();

	scroll_h_.draw();
	scroll_v_.draw();

	// draw.stop();
}

LinenumView::LinenumView(FileView &parent) : Widget(), parent_(parent) {}

void LinenumView::draw() {
	TextShader::globals.frame_offset_px = pos();

	TextShader::use(buf_);
	TextShader::update_uniforms();
	parent_.draw_lines(render_range_);

	TextShader::globals.is_foreground = true;
	TextShader::update_uniforms();
	parent_.draw_lines(render_range_);
}

ContentView::ContentView(FileView &parent) : Widget(), parent_(parent) {}

void ContentView::get_mouse_char(ivec2 mouse, size_t &buf_mouse_char, ivec2 &mouse_buf_pos) const {
	auto buf_start_char = parent_.get_line_start(parent_.buf_lines_.x, parent_.line_ends_);
	auto first_visible_char = parent_.scroll_ / TextShader::font().size;
	// printf("Line offset: %ld\n", first_visible_buf_line);

	auto hidden_pixels = parent_.scroll_ % TextShader::font().size;
	auto content_mouse_pos = mouse - pos();
	mouse_buf_pos = (content_mouse_pos + hidden_pixels) / TextShader::font().size + first_visible_char;

	mouse_buf_pos.y = std::min(mouse_buf_pos.y, (int)parent_.line_ends_.size() - 1);
	auto mouse_line_len = parent_.get_line_len(mouse_buf_pos.y, parent_.line_ends_);
	mouse_buf_pos.x = std::min(mouse_buf_pos.x, (int)mouse_line_len - 1);

	auto mouse_char_offset = parent_.get_line_start(mouse_buf_pos.y, parent_.line_ends_) + mouse_buf_pos.x;
	buf_mouse_char = mouse_char_offset - buf_start_char;
}

bool ContentView::on_mouse_button(ivec2 mouse, int button, int action, int mods) {
	// if (pressed()) {
	// 	auto mouse_char = get_mouse_char(mouse);
	// 	selection = {mouse_char, mouse_char};
	// } else {
	// 	selecting = false;
	// }
	return false;
}

bool ContentView::on_cursor_pos(ivec2 mouse) {
	if (!pressed()) {
		return false;
	}

	if (parent_.line_ends_.empty()) {
		return false;
	}

	size_t buf_mouse_char;
	ivec2 mouse_buf_pos;
	get_mouse_char(mouse, buf_mouse_char, mouse_buf_pos);

	// printf("first buf line: %d, first visible buf line: %d, Mouse position in buffer: %d %d\n", parent_.buf_lines_.x, first_visible_char.y - parent_.buf_lines_.x, mouse_buf_pos.x, mouse_buf_pos.y);
	// printf("buf_start_char: %zu, buf_mouse_char: %zu\n", buf_start_char, buf_mouse_char);

	// TODO don't highlight if mouse not touching a char. rn pos is clamped to the line length
	TextShader::CharStyle style {
				{},
				mouse_buf_pos,
				vec4{100, 200, 200, 255},
				vec4{200, 200, 200, 100}
	};
	glBindBuffer(GL_ARRAY_BUFFER, buf_.vbo_style);
	glBufferSubData(GL_ARRAY_BUFFER, buf_mouse_char * sizeof(TextShader::CharStyle), sizeof(TextShader::CharStyle), &style);
	return false;
}

void ContentView::draw() {
	TextShader::globals.frame_offset_px = pos();

	TextShader::use(buf_);
	TextShader::update_uniforms();
	parent_.draw_lines(render_range_);

	TextShader::globals.is_foreground = true;
	TextShader::update_uniforms();
	parent_.draw_lines(render_range_);
}

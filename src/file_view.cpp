#include <iostream>

#include "file_view.h"

#include "parser.h"
#include "../shaders/text_shader.h"
#include "util.h"

using namespace glm;





FileView::FileView(Widget &parent, ivec2 pos, ivec2 size, const char *path, const TextShader &text_shader)
	: Widget(&parent, pos, size), file_(path), text_shader_(text_shader), line_height_(text_shader.font().size.y)
	, scrollbar_(*this, {pos.x + size.x - 30, pos.y}, {30, size.y}, [this](double p){scroll_cb(p);}) {

}

int FileView::open() {
	{
		Timeit file_open_timeit("File Open");
		if (file_.open() != 0) {
			std::cerr << "Failed to open file_\n";
			return -1;
		}
		if (file_.mmap() != 0) {
			std::cerr << "Failed to mmap file\n";
			return -1;
		}
	}

	TextShader::create_buffers(vao_, vbo_text_, vbo_style_, MAX_VRAM_USAGE);

	add_child(&scrollbar_);

	parse();
	return 0;
}

int FileView::parse() {
	size_t num_threads = 1;
	size_t total_size = file_.size();
	std::cout << "Parsing " << total_size / 1024 / 1024 << " MiB\n";
	Timeit parse_timeit("Parse");
	auto starts = find_newlines(file_.mapped_data(), total_size, num_threads);
	line_starts_.reserve(starts.size());
	for (size_t i = 0; i < starts.size(); i++) {
		line_starts_.emplace_back(starts[i], false);
	}
	parse_timeit.stop();
	std::cout << "Found " << line_starts_.size() << " newlines\n";
	return 0;
}

int FileView::update_buffer() {
	// first and last line visible on screen
	ivec2 screen_lines = ivec2{scroll_.y, scroll_.y + size().y} / line_height_;

	if (screen_lines.x >= buf_lines_.x && screen_lines.y <= buf_lines_.y) {
		// If the visible lines are within the current buffer, no need to update
		return 0;
	}

	Timeit update_timeit("Update Buffer");

	size_t middle_line = (screen_lines.x + screen_lines.y) / 2;
	size_t min_num_lines = (screen_lines.y - screen_lines.x) / 2 + OVERSCAN_LINES;

	buf_lines_.x = middle_line <= min_num_lines ? 0 : middle_line - min_num_lines;
	buf_lines_.y = middle_line + min_num_lines >= line_starts_.size() ? line_starts_.size() - 1 : middle_line + min_num_lines;

	glBindBuffer(GL_ARRAY_BUFFER, vbo_text_);
	const uint8_t *ptr;
	size_t num_chars = 0;
	if (!line_starts_[buf_lines_.x].alternate && !line_starts_[buf_lines_.y].alternate) {
		ptr = file_.mapped_data();
		num_chars = line_starts_[buf_lines_.y].start - line_starts_[buf_lines_.x].start;
		glBufferSubData(GL_ARRAY_BUFFER, 0, num_chars, ptr + line_starts_[buf_lines_.x].start);
	} else if (line_starts_[buf_lines_.x].alternate && line_starts_[buf_lines_.y].alternate) {
		ptr = file_.tailed_data();
		num_chars = line_starts_[buf_lines_.y].start - line_starts_[buf_lines_.x].start;
		glBufferSubData(GL_ARRAY_BUFFER, 0, num_chars, ptr + line_starts_[buf_lines_.x].start);
	} else {
		assert(!line_starts_[buf_lines_.x].alternate && line_starts_[buf_lines_.y].alternate);
		ptr = file_.mapped_data();
		auto line = buf_lines_.x;
		while (!line_starts_[line].alternate) {
			line++;
		}
		num_chars = line_starts_[line].start - line_starts_[buf_lines_.x].start;
		glBufferSubData(GL_ARRAY_BUFFER, 0, num_chars, ptr + line_starts_[buf_lines_.x].start);

		ptr = file_.tailed_data();
		size_t num_chars2 = line_starts_[buf_lines_.y].start - line_starts_[line].start;
		glBufferSubData(GL_ARRAY_BUFFER, num_chars, num_chars2, ptr + line_starts_[line].start);
		num_chars += num_chars2;
	}


	glBindBuffer(GL_ARRAY_BUFFER, vbo_style_);
	std::vector<TextShader::CharStyle> styles;
	styles.reserve(num_chars);
	for (size_t i = 0; i < num_chars; i++) {
		styles.push_back({
			{},
			vec4{200, 200, 200, 255},
			vec4{100, 100, 100, 100}
		});
	}
	glBufferSubData(GL_ARRAY_BUFFER, 0, styles.size() * sizeof(TextShader::CharStyle), styles.data());

	update_timeit.stop();
	printf("Buffer size: %zu\n", num_chars * (sizeof(TextShader::CharStyle) + sizeof(uint8_t)));
	return 0;
}

// void FileView::set_viewport(uvec4 rect) {
// 	// rect_ = rect;
// }

void FileView::on_resize() {
    text_shader_.set_viewport({}, size());
	scrollbar_.resize({pos().x + size().x - 30, pos().y}, {30, size().y});
	update_scrollbar();
}

void FileView::update_scrollbar() {
	scrollbar_.set(scroll_.y, size().y, (line_starts_.size() - 1) * line_height_);
}

void FileView::scroll_cb(double percent) {
	scroll_.y = (int)(percent * (line_starts_.size() - 1) * line_height_);
	scroll_.y = std::max(scroll_.y, 0);
	scroll_.y = std::min(scroll_.y, (int)(line_starts_.size() - 1) * line_height_);
	update_scrollbar();
}

void FileView::scroll(ivec2 scroll) {
	scroll.x *= text_shader_.font().size.x;
	scroll.y *= line_height_;
	scroll *= -3;

	scroll_.x += std::max(scroll.x, -static_cast<int>(scroll_.x));
	scroll_.y += std::max(scroll.y, -static_cast<int>(scroll_.y));

	update_scrollbar();
}

void FileView::draw_lines(size_t first, size_t last, size_t buf_offset) {
	for (size_t line_offset = first; line_offset < last; line_offset++) {
		auto line = line_starts_[line_offset].start - buf_offset;
		auto next_line = line_starts_[line_offset+1].start - buf_offset;
		text_shader_.set_line_index(line_offset);
		glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, next_line - line, line);
	}
}

void FileView::draw() {
	glEnable(GL_SCISSOR_TEST);
	glScissor(pos().x, pos().y, size().x, size().y);

	text_shader_.use();
	glBindVertexArray(vao_);

	// auto scroll_height = size().y - scrollbar_.size().y;
	// auto scroll_pos = scrollbar_.pos().y - pos().y;
	// float scroll_percent = (float)scroll_pos / (float)scroll_height;
	//
	// scroll_.y = (int)(scroll_percent * (line_starts_.size() - 1) * line_height_);

	text_shader_.set_frame_offset(pos());
	text_shader_.set_scroll_offset(scroll_);
	update_buffer();

	// Timeit draw("Draw");
	auto buf_offset = line_starts_[buf_lines_.x].start;
	ivec2 screen_lines = ivec2{scroll_.y, scroll_.y + size().y} / line_height_;
	screen_lines.x = std::max(screen_lines.x, buf_lines_.x);
	screen_lines.y = std::min(screen_lines.y, buf_lines_.y);

	text_shader_.set_is_foreground(false);
	draw_lines(screen_lines.x, screen_lines.y, buf_offset);

	text_shader_.set_is_foreground(true);
	draw_lines(screen_lines.x, screen_lines.y, buf_offset);

	// disable shader
	// glBindVertexArray(0);
	// glUseProgram(0);

	scrollbar_.draw();

	// draw.stop();
	glDisable(GL_SCISSOR_TEST);
}

#include <iostream>

#include "file_view.h"

#include "parser.h"
#include "text_shader.h"
#include "util.h"

using namespace glm;



void Scrollbar::on_hover() {
	color_ = {0, 150, 0, 255};
}

void Scrollbar::on_unhover() {
	color_ = {100, 0, 0, 255};
}

void Scrollbar::on_press() {
	color_ = {0, 0, 200, 255};
}

void Scrollbar::on_release() {
	color_ = {0, 150, 0, 255};
}

void Scrollbar::on_cursor_pos(glm::uvec2 pos) {
	printf("Scrollbar cursor pos: %u, %u\n", pos.x, pos.y);
}

void Scrollbar::draw() {
	gp_shader_.rect(pos(), pos() + size(), {100, hovered() * 255, pressed() * 255, 255});
}






FileView::FileView(uvec2 pos, uvec2 size, const char *path, const TextShader &text_shader, GPShader &gp_shader)
	: IWidget(pos, size), file_(path), text_shader_(text_shader), gp_shader_(gp_shader), line_height_(text_shader.font().size.y)
	, scrollbar_({pos.x + size.x - 16, pos.y}, {16, 20}, gp_shader) {
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
	uvec2 screen_lines = uvec2{scroll_.y, scroll_.y + rect_.z} / line_height_;

	if (screen_lines.x >= buf_lines_.x && screen_lines.y <= buf_lines_.y) {
		// If the visible lines are within the current buffer, no need to update
		return 0;
	}

	// Timeit update_timeit("Update Buffer");

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

	// update_timeit.stop();
	// printf("Buffer size: %zu\n", num_chars * (sizeof(TextShader::CharStyle) + sizeof(uint8_t)));
	return 0;
}

void FileView::set_viewport(uvec4 rect) {
	rect_ = rect;
}

void FileView::scroll(ivec2 scroll) {
	scroll.x *= text_shader_.font().size.x;
	scroll.y *= line_height_;
	scroll *= -3;

	scroll_.x += std::max(scroll.x, -static_cast<int>(scroll_.x));
	scroll_.y += std::max(scroll.y, -static_cast<int>(scroll_.y));
}

void FileView::draw_lines(size_t first, size_t last, size_t buf_offset) {
	for (size_t line_offset = first; line_offset < last; line_offset++) {
		auto line = line_starts_[line_offset].start - buf_offset;
		auto next_line = line_starts_[line_offset+1].start - buf_offset;
		text_shader_.set_line_index(line_offset);
		glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, next_line - line, line);
	}
}

void FileView::draw_scrollbar() {
	// Draw scrollbar
	float scrollbar_width = 16.0f;
	float total_lines = static_cast<float>(line_starts_.size());
	float visible_lines = static_cast<float>(rect_.w) / line_height_;
	float scroll_pos = static_cast<float>(scroll_.y) / line_height_;

	float bar_height = std::max(visible_lines / total_lines * rect_.w, 20.0f);
	float bar_y = (scroll_pos / total_lines) * rect_.w;
	// scrollbar_.resize(
		// {rect_.x + rect_.z - scrollbar_width, rect_.y + bar_y},
		// {scrollbar_width, bar_height}
	// );
	scrollbar_.draw();
}

void FileView::draw() {
	text_shader_.use();
	glBindVertexArray(vao_);
	text_shader_.set_scroll_offset(scroll_);
	update_buffer();

	// Timeit draw("Draw");
	auto buf_offset = line_starts_[buf_lines_.x].start;
	uvec2 screen_lines = uvec2{scroll_.y, scroll_.y + rect_.z} / line_height_;
	screen_lines.x = std::max(screen_lines.x, buf_lines_.x);
	screen_lines.y = std::min(screen_lines.y, buf_lines_.y);

	text_shader_.set_is_foreground(false);
	draw_lines(screen_lines.x, screen_lines.y, buf_offset);

	text_shader_.set_is_foreground(true);
	draw_lines(screen_lines.x, screen_lines.y, buf_offset);

	// disable shader
	// glBindVertexArray(0);
	// glUseProgram(0);

	draw_scrollbar();

	// draw.stop();
}

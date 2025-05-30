#include <iostream>

#include "file_view.h"

#include "parser.h"
#include "../shaders/text_shader.h"
#include "util.h"

using namespace glm;

FileView::FileView(const char *path)
	: file_(path), scroll_h_(true, [this](double p){scroll_h_cb(p);}), scroll_v_(false, [this](double p){scroll_v_cb(p);}) {

	add_child(&scroll_h_);
	add_child(&scroll_v_);
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

	TextShader::create_buffers(content_buf_, MAX_VRAM_USAGE);
	TextShader::create_buffers(linenum_buf_, 1024 * 1024); // 1 MiB for line numbers

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
	longest_line_ = 0;
	size_t prev_start = 0;
	for (size_t i = 0; i < starts.size(); i++) {
		line_starts_.emplace_back(starts[i], false);
		size_t line_length = starts[i] - prev_start;
		longest_line_ = std::max(longest_line_, line_length);
		prev_start = starts[i];
	}
	parse_timeit.stop();
	std::cout << "Found " << line_starts_.size() << " newlines\n";
	return 0;
}

int FileView::update_content_buffer() {
	// first and last line visible on screen
	ivec2 screen_lines = ivec2{scroll_.y, scroll_.y + size().y} / TextShader::font().size.y;

	if (screen_lines.x >= buf_lines_.x && screen_lines.y <= buf_lines_.y) {
		// If the visible lines are within the current buffer, no need to update
		return 0;
	}

	Timeit update_timeit("Update content buffer");

	size_t middle_line = (screen_lines.x + screen_lines.y) / 2;
	size_t min_num_lines = (screen_lines.y - screen_lines.x) / 2 + OVERSCAN_LINES;

	buf_lines_.x = middle_line <= min_num_lines ? 0 : middle_line - min_num_lines;
	buf_lines_.y = middle_line + min_num_lines >= line_starts_.size() ? line_starts_.size() - 1 : middle_line + min_num_lines;

	glBindBuffer(GL_ARRAY_BUFFER, content_buf_.vbo_text);
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


	glBindBuffer(GL_ARRAY_BUFFER, content_buf_.vbo_style);
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
	printf("Content buffer size: %zu\n", num_chars * (sizeof(TextShader::CharStyle) + sizeof(uint8_t)));
	return 0;
}

int FileView::update_linenum_buffer() {
	// first and last line visible on screen
	ivec2 screen_lines = ivec2{scroll_.y, scroll_.y + size().y} / TextShader::font().size.y;
	//
	// if (screen_lines.x >= buf_lines_.x && screen_lines.y <= buf_lines_.y) {
	// 	// If the visible lines are within the current buffer, no need to update
	// 	return 0;
	// }

	Timeit update_timeit("Update linenum buffer");

	size_t middle_line = (screen_lines.x + screen_lines.y) / 2;
	size_t min_num_lines = (screen_lines.y - screen_lines.x) / 2 + OVERSCAN_LINES;

	buf_lines_.x = middle_line <= min_num_lines ? 0 : middle_line - min_num_lines;
	buf_lines_.y = middle_line + min_num_lines >= line_starts_.size() ? line_starts_.size() - 1 : middle_line + min_num_lines;

	std::vector<uint8_t> linenum_text {};
	std::vector<TextShader::CharStyle> styles {};
	for (uint line_idx = buf_lines_.x; line_idx < buf_lines_.y; line_idx++) {
		std::string linenum_str = std::to_string(line_idx + 1);
		linenum_text.insert(linenum_text.end(), linenum_str.begin(), linenum_str.end());

		for (size_t i = 0; i < linenum_str.size(); i++) {
			styles.push_back({
				{},
				line_idx,
				vec4{200, 200, 200, 255},
				vec4{100, 100, 100, 100}
			});
		}
	}
	glBindBuffer(GL_ARRAY_BUFFER, content_buf_.vbo_text);
	glBufferSubData(GL_ARRAY_BUFFER, 0, linenum_text.size(), linenum_text.data());

	glBindBuffer(GL_ARRAY_BUFFER, content_buf_.vbo_style);
	glBufferSubData(GL_ARRAY_BUFFER, 0, styles.size() * sizeof(TextShader::CharStyle), styles.data());

	update_timeit.stop();
	printf("linenum buffer size: %zu\n", linenum_text.size() * (sizeof(TextShader::CharStyle) + sizeof(uint8_t)));
	return 0;
}

void FileView::on_resize() {
	scroll_h_.resize({pos().x, pos().y + size().y - 30}, {size().x, 30});
	scroll_v_.resize({pos().x + size().x - 30, pos().y}, {30, size().y});
	update_scrollbar();
}

void FileView::update_scrollbar() {
	scroll_h_.set(scroll_.x, size().x, longest_line_ * TextShader::font().size.x);
	scroll_v_.set(scroll_.y, size().y, (line_starts_.size() - 1) * TextShader::font().size.y);
}

void FileView::scroll_h_cb(double percent) {
	scroll_.x = (int)(percent * longest_line_ * TextShader::font().size.x);
	scroll_.x = std::max(scroll_.x, 0);
	scroll_.x = std::min(scroll_.x, (int)longest_line_ * TextShader::font().size.x);
	update_scrollbar();
}

void FileView::scroll_v_cb(double percent) {
	scroll_.y = (int)(percent * (line_starts_.size() - 1) * TextShader::font().size.y);
	scroll_.y = std::max(scroll_.y, 0);
	scroll_.y = std::min(scroll_.y, (int)(line_starts_.size() - 1) * TextShader::font().size.y);
	update_scrollbar();
}

void FileView::scroll(ivec2 scroll) {
	scroll *= TextShader::font().size;
	scroll *= -3;

	scroll_.x += std::max(scroll.x, -scroll_.x);
	scroll_.y += std::max(scroll.y, -scroll_.y);

	update_scrollbar();
}


void FileView::draw_lines(size_t first, size_t last, size_t buf_offset, bool linenum) {
	for (size_t line_offset = first; line_offset < last; line_offset++) {
		auto line = line_starts_[line_offset].start - buf_offset;
		auto next_line = line_starts_[line_offset+1].start - buf_offset;
		TextShader::globals.line_idx = line_offset;
		TextShader::update_uniforms();
		glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, next_line - line, line);
	}
}

void FileView::draw_linenums(size_t first, size_t last, size_t buf_offset) {
	TextShader::use(linenum_buf_);
	TextShader::update_uniforms();
	draw_lines(first, last, buf_offset, true);

	TextShader::globals.is_foreground = true;
	TextShader::update_uniforms();
	draw_lines(first, last, buf_offset, true);
}

void FileView::draw_content(size_t first, size_t last, size_t buf_offset) {
	TextShader::use(content_buf_);
	TextShader::update_uniforms();
	draw_lines(first, last, buf_offset, false);

	TextShader::globals.is_foreground = true;
	TextShader::update_uniforms();
	draw_lines(first, last, buf_offset, false);
}

void FileView::draw() {
	TextShader::globals.set_viewport(pos(), size());
	TextShader::globals.frame_offset_px = pos();
	TextShader::globals.scroll_offset_px = scroll_;
	TextShader::globals.is_foreground = false;


	update_content_buffer();
	update_linenum_buffer();

	// Timeit draw("Draw");
	auto buf_offset = line_starts_[buf_lines_.x].start;
	ivec2 screen_lines = ivec2{scroll_.y, scroll_.y + size().y} / TextShader::font().size.y;
	screen_lines.x = std::max(screen_lines.x, buf_lines_.x);
	screen_lines.y = std::min(screen_lines.y, buf_lines_.y);

	draw_content(buf_lines_.x, buf_lines_.y, buf_offset);
	draw_linenums(buf_lines_.x, buf_lines_.y, buf_offset);

	scroll_h_.draw();
	scroll_v_.draw();

	// draw.stop();
}

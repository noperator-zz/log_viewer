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

static size_t linenum_len(size_t line) {
	return (size_t)log10(line) + 1;
}

static size_t linenum_buf_offset(size_t line) {
	// return the length of `line` line numbers
	size_t offset = 0;
	size_t num_digits;
	while (line > 0) {
		num_digits = linenum_len(line);
		auto num_line_of_this_length = line + 1 - (size_t)pow(10, num_digits - 1);
		offset += num_digits * num_line_of_this_length;
		line -= num_line_of_this_length;
	}
	return offset;
}

int FileView::update_buffers() {
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

	auto content_styles = std::unique_ptr<TextShader::CharStyle[]>(new TextShader::CharStyle[num_chars]);
	size_t c = 0;
	for (uint line_idx = buf_lines_.x; line_idx < buf_lines_.y; line_idx++) {
		size_t line_len = line_starts_[line_idx + 1].start - line_starts_[line_idx].start;
		for (size_t i = 0; i < line_len; i++) {
			content_styles[c] = {
				{},
				line_idx,
				vec4{200, 200, 200, 255},
				vec4{100, 100, 100, 100}
			};
			c++;
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, content_buf_.vbo_style);
	glBufferSubData(GL_ARRAY_BUFFER, 0, num_chars * sizeof(TextShader::CharStyle), content_styles.get());

	auto num_lines = buf_lines_.y - buf_lines_.x;
	// max number of characters for all line numbers
	auto linenum_chars = linenum_len(buf_lines_.y) * num_lines;

	auto linenum_text = std::unique_ptr<uint8_t[]>(new uint8_t[linenum_chars + 1]); // +1 for the null terminator added by sprintf
	auto linenum_styles = std::unique_ptr<TextShader::CharStyle[]>(new TextShader::CharStyle[linenum_chars]);

	c = 0;
	for (uint line_idx = buf_lines_.x; line_idx < buf_lines_.y; line_idx++) {
		// std::string linenum_str = std::to_string(line_idx + 1);
		// linenum_text.insert(linenum_text.end(), linenum_str.begin(), linenum_str.end());
		auto line_len = sprintf((char*)&linenum_text[c], "%zu", line_idx + 1);
		// auto line_len = linenum_len(line_idx + 1); // TODO

		for (size_t i = 0; i < line_len; i++) {
			linenum_styles[c] = {
				{},
				line_idx,
				vec4{200, 200, 200, 255},
				vec4{100, 100, 100, 100}
			};
			c++;
		}
	}
	glBindBuffer(GL_ARRAY_BUFFER, linenum_buf_.vbo_text);
	glBufferSubData(GL_ARRAY_BUFFER, 0, linenum_chars, linenum_text.get());

	glBindBuffer(GL_ARRAY_BUFFER, linenum_buf_.vbo_style);
	glBufferSubData(GL_ARRAY_BUFFER, 0, linenum_chars * sizeof(TextShader::CharStyle), linenum_styles.get());

	update_timeit.stop();
	printf("Content buffer size: %zu\n", num_chars * (sizeof(TextShader::CharStyle) + sizeof(uint8_t)));
	printf("Linenum buffer size: %zu\n", linenum_chars * (sizeof(TextShader::CharStyle) + sizeof(uint8_t)));
	return 0;
}

void FileView::draw_lines(bool is_linenum) {
	auto first = buf_lines_.x;
	auto last = buf_lines_.y;
	size_t buf_offset {};
	size_t line {};
	size_t next_line {};
	size_t line_len {};
	size_t line_overflow {};
	if (is_linenum) {
		line_len = linenum_len(first + 1);
		line_overflow = pow(10, line_len);
		buf_offset = line = next_line = linenum_buf_offset(first);
	} else {
		buf_offset = line = line_starts_[first].start;
	}

	for (size_t line_offset = first+1; line_offset < last; line_offset++) {
		if (is_linenum) {
			if (line_offset == line_overflow) {
				line_len++;
				line_overflow *= 10;
			}
			next_line += line_len;
		} else {
			next_line = line_starts_[line_offset].start;
		}
		glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, next_line - line, line - buf_offset);
		line = next_line;
	}
}

void FileView::draw_linenums() {
	TextShader::use(linenum_buf_);
	TextShader::update_uniforms();
	draw_lines(true);

	TextShader::globals.is_foreground = true;
	TextShader::update_uniforms();
	draw_lines(true);
}

void FileView::draw_content() {
	TextShader::use(content_buf_);
	TextShader::update_uniforms();
	draw_lines(false);

	TextShader::globals.is_foreground = true;
	TextShader::update_uniforms();
	draw_lines(false);
}

void FileView::draw() {
	TextShader::globals.frame_offset_px = pos();
	TextShader::globals.scroll_offset_px = scroll_;
	TextShader::globals.is_foreground = false;


	update_buffers();

	Timeit draw("Draw");

	TextShader::globals.set_viewport(pos() + ivec2{-100, 0}, size());
	draw_content();

	TextShader::globals.set_viewport(pos(), size());
	draw_linenums();

	scroll_h_.draw();
	scroll_v_.draw();

	draw.stop();
}

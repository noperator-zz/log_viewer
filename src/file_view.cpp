#include <iostream>

#include "file_view.h"

#include "parser.h"
#include "text_shader.h"
#include "util.h"

using namespace glm;

FileView::FileView(const char *path, const TextShader &text_shader) : file_(path), text_shader_(text_shader), line_height(text_shader.font().size.y) {
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

	parse();
	return 0;
}

int FileView::parse() {
	size_t num_threads = 1;
	size_t total_size = file_.size();
	std::cout << "Parsing " << total_size / 1024 / 1024 << " MiB\n";
	Timeit parse_timeit("Parse");
	auto starts = find_newlines(file_.mapped_data(), total_size, num_threads);
	line_starts.reserve(starts.size());
	for (size_t i = 0; i < starts.size(); i++) {
		line_starts.emplace_back(starts[i], false);
	}
	parse_timeit.stop();
	std::cout << "Found " << line_starts.size() << " newlines\n";
	return 0;
}

int FileView::update_buffer() {
	// first and last line visible on screen
	uvec2 screen_lines = uvec2{scroll.y, scroll.y + rect_.z} / line_height;

	if (screen_lines.x >= buf_lines.x && screen_lines.y <= buf_lines.y) {
		// If the visible lines are within the current buffer, no need to update
		return 0;
	}

	// Timeit update_timeit("Update Buffer");

	size_t middle_line = (screen_lines.x + screen_lines.y) / 2;
	size_t min_num_lines = (screen_lines.y - screen_lines.x) / 2 + OVERSCAN_LINES;

	buf_lines.x = middle_line <= min_num_lines ? 0 : middle_line - min_num_lines;
	buf_lines.y = middle_line + min_num_lines >= line_starts.size() ? line_starts.size() - 1 : middle_line + min_num_lines;

	glBindBuffer(GL_ARRAY_BUFFER, vbo_text_);
	const uint8_t *ptr;
	size_t num_chars = 0;
	if (!line_starts[buf_lines.x].alternate && !line_starts[buf_lines.y].alternate) {
		ptr = file_.mapped_data();
		num_chars = line_starts[buf_lines.y].start - line_starts[buf_lines.x].start;
		glBufferSubData(GL_ARRAY_BUFFER, 0, num_chars, ptr + line_starts[buf_lines.x].start);
	} else if (line_starts[buf_lines.x].alternate && line_starts[buf_lines.y].alternate) {
		ptr = file_.tailed_data();
		num_chars = line_starts[buf_lines.y].start - line_starts[buf_lines.x].start;
		glBufferSubData(GL_ARRAY_BUFFER, 0, num_chars, ptr + line_starts[buf_lines.x].start);
	} else {
		assert(!line_starts[buf_lines.x].alternate && line_starts[buf_lines.y].alternate);
		ptr = file_.mapped_data();
		auto line = buf_lines.x;
		while (!line_starts[line].alternate) {
			line++;
		}
		num_chars = line_starts[line].start - line_starts[buf_lines.x].start;
		glBufferSubData(GL_ARRAY_BUFFER, 0, num_chars, ptr + line_starts[buf_lines.x].start);

		ptr = file_.tailed_data();
		size_t num_chars2 = line_starts[buf_lines.y].start - line_starts[line].start;
		glBufferSubData(GL_ARRAY_BUFFER, num_chars, num_chars2, ptr + line_starts[line].start);
		num_chars += num_chars2;
	}


	glBindBuffer(GL_ARRAY_BUFFER, vbo_style_);
	std::vector<TextShader::CharStyle> styles;
	styles.reserve(num_chars);
	for (size_t i = 0; i < num_chars; i++) {
		styles.push_back({
			{},
			vec4{0.9, 0.9, 0.9, 1},
			vec4{0.3, 0.3, 0.3, 0.5}
		});
	}
	glBufferSubData(GL_ARRAY_BUFFER, 0, styles.size() * sizeof(TextShader::CharStyle), styles.data());

	// update_timeit.stop();
	// printf("Buffer size: %zu\n", num_chars * (sizeof(TextShader::CharStyle) + sizeof(uint8_t)));
	return 0;
}

void FileView::set_viewport(uvec4 rect) {
	rect_ = rect;
	text_shader_.set_viewport(rect_);
}

void FileView::draw_lines(size_t first, size_t last, size_t buf_offset) {
	for (size_t line_offset = first; line_offset < last; line_offset++) {
		auto line = line_starts[line_offset].start - buf_offset;
		auto next_line = line_starts[line_offset+1].start - buf_offset;
		text_shader_.set_line_index(line_offset);
		glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, next_line - line, line);
	}
}

int FileView::draw() {
	scroll.x += 1;

	text_shader_.use(vao_);
	// glBindVertexArray(vao_);
	text_shader_.set_scroll_offset(scroll);
	update_buffer();

	// Timeit draw("Draw");
	auto buf_offset = line_starts[buf_lines.x].start;
	uvec2 screen_lines = uvec2{scroll.y, scroll.y + rect_.z} / line_height;
	screen_lines.x = std::max(screen_lines.x, buf_lines.x);
	screen_lines.y = std::min(screen_lines.y, buf_lines.y);

	text_shader_.set_is_foreground(false);
	draw_lines(screen_lines.x, screen_lines.y, buf_offset);

	text_shader_.set_is_foreground(true);
	draw_lines(screen_lines.x, screen_lines.y, buf_offset);

	// draw.stop();
	return 0;
}

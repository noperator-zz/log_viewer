#include <iostream>

#include "file_view.h"

#include "parser.h"
#include "text_shader.h"
#include "util.h"

FileView::FileView(const char *path) : file_(path) {
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

	TextShader::create_buffers(vao_, vbo_text_, vbo_style_);

	parse();
	return 0;
}

int FileView::parse() {
	size_t num_threads = 1;
	size_t total_size = file_.size();
	std::cout << "Parsing " << total_size / 1024 / 1024 << " MiB\n";
	Timeit parse_timeit("Parse");
	line_starts = find_newlines(file_.data(), total_size, num_threads);
	parse_timeit.stop();
	std::cout << "Found " << line_starts.size() << " newlines\n";
	return 0;
}

int FileView::update_buffer() {



	glBindBuffer(GL_ARRAY_BUFFER, vbo_text_);
	glBufferData(GL_ARRAY_BUFFER, std::min(file_.size(), 1024 * 1024ULL), file_.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_style_);
	glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
	return 0;
}

int FileView::draw(TextShader &shader) {
	shader.use(vao_);
	// glBindVertexArray(vao_);
	shader.set_scroll_offset(scroll.x, scroll.y);
	update_buffer();

	Timeit draw("Draw");
	size_t line_offset = 0;
	size_t visible_lines = 10;
	for (size_t line_idx = line_offset; line_idx < line_offset + visible_lines; line_idx++) {
		auto line = line_starts[line_idx];
		auto next_line = line_starts[line_idx+1];
		shader.set_line_index(line_idx);
		glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, next_line - line, line);
	}
	draw.stop();
	return 0;
}

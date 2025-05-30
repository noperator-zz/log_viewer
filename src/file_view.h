#pragma once

#include <functional>
#include <utility>
#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "file.h"
#include "gp_shader.h"
#include "scrollbar.h"
#include "text_shader.h"
#include "widget.h"

class FileView : public Widget {
	static constexpr size_t MAX_VRAM_USAGE = 256 * 1024 * 1024;
	// static constexpr size_t USAGE_PER_CHAR = sizeof(TextShader::CharStyle) + sizeof(uint8_t);
	// static constexpr size_t MAX_CHARS = MAX_VRAM_USAGE / USAGE_PER_CHAR;

	static constexpr size_t OVERSCAN_LINES = 1;

	File file_;
	Scrollbar scroll_h_;
	Scrollbar scroll_v_;
	TextShader::Buffer content_buf_ {};
	TextShader::Buffer linenum_buf_ {};

	// First and last lines in the buffer
	glm::ivec2 buf_lines_ {};
	glm::ivec2 scroll_ {};

	struct Line {
		size_t start: 63;
		bool alternate: 1;
	};

	std::vector<Line> line_starts_ {};
	size_t longest_line_  {};

	void on_resize() override;
	void update_scrollbar();

	int parse();
	void draw_lines(size_t first, size_t last, size_t buf_offset);
	void draw_linenums(size_t first, size_t last, size_t buf_offset);
	void draw_content(size_t first, size_t last, size_t buf_offset);
	void scroll_h_cb(double percent);
	void scroll_v_cb(double percent);
public:

	FileView(const char *path);

	int open();
	int update_buffer();
	void scroll(glm::ivec2 scroll);

	void draw() override;
};

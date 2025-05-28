#pragma once

#include <functional>
#include <utility>
#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "file.h"
#include "gp_shader.h"
#include "text_shader.h"
#include "widget.h"

class Scrollbar : public Widget {
	class Thumb : public Widget {
		friend class Scrollbar;
		glm::u8vec4 color_ {100, 0, 0, 255};
		GPShader &gp_shader_;
		std::function<void(double)> scroll_cb_;

		void on_hover() override;
		void on_unhover() override;
		void on_press() override;
		void on_release() override;
		// void on_cursor_pos(glm::ivec2 pos) override;
		void on_drag(glm::ivec2 offset) override;

		void draw() override;
	public:
		Thumb(Widget &parent, glm::ivec2 pos, glm::ivec2 size, GPShader &gp_shader, std::function<void(double)> scroll_cb)
			: Widget(&parent, pos, size), gp_shader_(gp_shader), scroll_cb_(std::move(scroll_cb)) {}
	};

	Thumb thumb_;
	GPShader &gp_shader_;
	double position_percent_ {};
	double visible_percent_ {};

public:
	Scrollbar(Widget &parent, glm::ivec2 pos, glm::ivec2 size, GPShader &gp_shader, std::function<void(double)> scroll_cb)
		: Widget(&parent, pos, size), thumb_(*this, pos, {30, 50}, gp_shader, scroll_cb), gp_shader_(gp_shader) {
		add_child(&thumb_);
	}

	void on_resize() override;
	void draw() override;

	void resize_thumb();
	void set(size_t position, size_t visible_extents, size_t total_extents);
};

class FileView : public Widget {
	static constexpr size_t MAX_VRAM_USAGE = 256 * 1024 * 1024;
	// static constexpr size_t USAGE_PER_CHAR = sizeof(TextShader::CharStyle) + sizeof(uint8_t);
	// static constexpr size_t MAX_CHARS = MAX_VRAM_USAGE / USAGE_PER_CHAR;

	static constexpr size_t OVERSCAN_LINES = 1;

	File file_;
	const TextShader &text_shader_;
	GPShader &gp_shader_;
	Scrollbar scrollbar_;
	// glm::uvec4 rect_ {};
	GLuint vao_ {};
	GLuint vbo_text_ {};
	GLuint vbo_style_ {};

	// First and last lines in the buffer
	glm::ivec2 buf_lines_ {};
	glm::ivec2 scroll_ {};
	int line_height_ {};

	struct Line {
		size_t start: 63;
		bool alternate: 1;
	};

	std::vector<Line> line_starts_ {};

	void on_resize() override;
	void update_scrollbar();

	int parse();
	void draw_lines(size_t first, size_t last, size_t buf_offset);
	void scroll_cb(double percent);
public:

	FileView(Widget &parent, glm::ivec2 pos, glm::ivec2 size, const char *path, const TextShader &text_shader, GPShader &gp_shader);

	int open();
	int update_buffer();
	// void set_viewport(glm::uvec4 rect);
	void scroll(glm::ivec2 scroll);

	void draw() override;
};

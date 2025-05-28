#pragma once

#include <glm/glm.hpp>

#include "font.h"
#include "shader.h"

class TextShader {
public:
	struct __attribute__((packed)) CharStyle {
		union {
			struct {
				uint8_t bold : 1;
				uint8_t italic : 1;
				uint8_t underline : 1;
				uint8_t strikethrough : 1;
			};
			uint8_t style;
		};
		glm::u8vec4 fg;
		glm::u8vec4 bg;
	};

private:
	Shader shader_;
	const Font &font_;
	GLint frame_offset_loc_ {};
	GLint scroll_offset_loc_ {};
	GLint line_index_loc_ {};
	GLint line_height_loc_ {};
	GLint is_foreground_loc_ {};

public:
	TextShader(const Font &font);

	static void create_buffers(GLuint &vao, GLuint &vbo_text, GLuint &vbo_style, size_t total_size);

	int setup();
	void use() const;
	void set_viewport(glm::ivec2 pos, glm::ivec2 size) const;
	void set_frame_offset(glm::ivec2 offset) const;
	void set_scroll_offset(glm::ivec2 offset) const;
	void set_line_index(int line_index) const;
	void set_line_height(int line_height) const;
	void set_is_foreground(bool is_foreground) const;
	const Font &font() const {
		return font_;
	}
};

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
		glm::vec3 fg;
		glm::vec3 bg;
	};

private:
	Shader shader_;
	const Font &font_;
	GLint scroll_offset_loc_ {};
	GLint line_index_loc_ {};
	GLint line_height_loc_ {};

public:
	TextShader(const Font &font);

	static void create_buffers(GLuint &vao, GLuint &vbo_text, GLuint &vbo_style, size_t total_size);

	int setup();
	void use(GLuint vao) const;
	void set_viewport(glm::uvec4 rect) const;
	void set_scroll_offset(glm::uvec2 scroll_offest) const;
	void set_line_index(glm::uint line_index) const;
	void set_line_height(glm::uint line_height) const;
	const Font &font() const {
		return font_;
	}
};

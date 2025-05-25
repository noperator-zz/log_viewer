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
	Font &font_;
	GLint scroll_offset_loc_ {};
	GLint line_index_loc_ {};

public:
	TextShader(Font &font);

	static void create_buffers(GLuint &vao, GLuint &vbo_text, GLuint &vbo_style);

	int setup();
	void use(GLuint vao) const;
	void set_viewport(int x, int y, int w, int h) const;
	void set_scroll_offset(float x, float y) const;
	void set_line_index(int line_index) const;
};

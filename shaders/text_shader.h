#pragma once

#include <memory>
#include <glm/glm.hpp>

#include "../src/font.h"
#include "../src/shader.h"

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
	static inline std::unique_ptr<TextShader> inst_;
	Shader shader_;
	const Font &font_;
	GLint frame_offset_loc_ {};
	GLint scroll_offset_loc_ {};
	GLint line_index_loc_ {};
	GLint is_foreground_loc_ {};

	TextShader(const Font &font);

	int setup();

public:
	static int init(const Font &font);
	static void create_buffers(GLuint &vao, GLuint &vbo_text, GLuint &vbo_style, size_t total_size);

	static void use();
	static void set_viewport(glm::ivec2 pos, glm::ivec2 size);
	static void set_frame_offset(glm::ivec2 offset);
	static void set_scroll_offset(glm::ivec2 offset);
	static void set_line_index(int line_index);
	static void set_is_foreground(bool is_foreground);
	static const Font &font();
};

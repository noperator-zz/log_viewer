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
			uint8_t style {};
		};
		glm::u8vec4 fg {};
		glm::u8vec4 bg {};
	};

	struct UniformGlobals {
		glm::mat4 u_proj;
		glm::vec2 glyph_size_px;
		glm::ivec2 scroll_offset_px;
		glm::ivec2 frame_offset_px;
		int line_idx;
		glm::uint atlas_cols;
		alignas(4) bool is_foreground;

		void set_viewport(glm::ivec2 pos, glm::ivec2 size);
	};

	struct Buffer {
		GLuint vao {};
		GLuint vbo_text {};
		GLuint vbo_style {};
		GLuint ubo_globals {};
	};

private:
	static inline std::unique_ptr<TextShader> inst_;
	Shader shader_;
	const Font &font_;
	GLint globals_idx_ {};
	// Buffer *active_buf_ {};

	TextShader(const Font &font);

	int setup();

public:
	static inline UniformGlobals globals {};
	static int init(const Font &font);
	static void create_buffers(Buffer &buf, size_t total_size);

	static void update_uniforms();
	static void use(Buffer &buf);
	static const Font &font();
};

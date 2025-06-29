#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>

#include "types.h"
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
		glm::uvec2 char_pos {};
		color fg {};
		color bg {};
	};

	struct UniformGlobals {
		glm::mat4 u_proj;
		glm::ivec2 glyph_size_px;
		glm::ivec2 scroll_offset_px;
		glm::ivec2 frame_offset_px;
		glm::uint atlas_cols;
		alignas(4) uint8_t z_order;

		void set_viewport(glm::ivec2 size);
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
	static void destroy_buffers(Buffer &buf);

	static void update_uniforms();
	static void use(const Buffer &buf);
	static void render(const Buffer &buf, std::string_view text, const CharStyle &style);
	static void render(const Buffer &buf, std::string_view text, const CharStyle &style, const std::vector<size_t> &indices, std::vector<glm::ivec2> &coords);
	static void draw(glm::ivec2 frame_offset, glm::ivec2 scroll_offset, size_t start, size_t count, uint8_t z);
	static const Font &font();
};

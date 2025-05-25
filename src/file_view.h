#pragma once

#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "file.h"
#include "text_shader.h"

class FileView {
	static constexpr size_t MAX_VRAM_USAGE = 256 * 1024 * 1024;
	static constexpr size_t USAGE_PER_CHAR = sizeof(TextShader::CharStyle) + sizeof(uint8_t);
	static constexpr size_t MAX_CHARS = MAX_VRAM_USAGE / USAGE_PER_CHAR;

	File file_;
	GLuint vao_ {};
	GLuint vbo_text_ {};
	GLuint vbo_style_ {};

	glm::u64vec2 buf_lines {};
	glm::u64vec2 scroll {};

	std::vector<size_t> line_starts {};

	int parse();

public:

	FileView(const char *path);
	int open();
	int update_buffer();
	int draw(TextShader &shader);
};

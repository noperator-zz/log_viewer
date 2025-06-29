#pragma once

#include <cstdint>
#include <ft2build.h>
#include <GL/glew.h>
#include <glm/vec2.hpp>

#include FT_FREETYPE_H

class Font {
public:
	static constexpr size_t num_glyphs = 256;
	static constexpr size_t num_faces = 4;

	glm::ivec2 size {};
private:
	const char *paths[4];
	GLuint tex_atlas_ {};

	int render(FT_Face face, size_t style_idx);

	static FT_Library ft;
public:
	static int init();

	Font(int size,
		const char *path_reg,
		const char *path_bold,
		const char *path_italic,
		const char *path_bold_italic
	);
	int load();

	GLuint tex_atlas() const {
		return tex_atlas_;
	}
};
